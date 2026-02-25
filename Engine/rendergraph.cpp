#include "rendergraph.hpp"
#include "rendergroup.hpp"
#include "framebuffer.hpp"
#include "static_meshpool.hpp"
#include "GL/glew.h"
#include "shader_program.hpp"
#include "compute_shader_program.hpp"
#include <unordered_set>
#include <algorithm>

RenderGraph::RenderGraph() {
	
}

//static void BindRenderTarget(FramebufferRenderTargetDescriptor target) {
	//Assert(false);
	
	//std::vector<glm::vec4> clearValues;
	//clearValues.resize(p.renderTarget.framebuffer->textureAttachments.size(), { -1, -1, -1, -1 });
	//for (auto& policy : p.renderTarget.loadPolicies) {
		//policy.
	//}
	//p.renderTarget.framebuffer->Clear(clearValues);
//}


void RenderGraph::Render() {
	if (dirty) Compile();

	for (auto& pass : renderOrder) {
		if (std::holds_alternative<ProcessedDrawPass>(pass)) {
			auto& p = std::get<ProcessedDrawPass>(pass);

			if (p.dependencyWriteMemoryBarrierBits != 0) {
				glMemoryBarrier(p.dependencyWriteMemoryBarrierBits);
			}

			p.bindRenderTarget();

			p.params.shader->Use();
			if (p.setUniforms.operator bool()) p.setUniforms(p.params.shader);

			if (p.params.blending) {
				glEnable(GL_BLEND);
			}
			else {
				glDisable(GL_BLEND);
			}
			glDepthMask(p.params.writeDepthBuffer);
			if (p.params.scissoringEnabled) {
				glEnable(GL_SCISSOR_TEST);
				auto c1 = p.params.scissorCorner1, c2 = p.params.scissorCorner2;
				glm::ivec2 size = c2 - c1;
				if (size.x < 0) {
					size.x *= -1;
					std::swap(c1.x, c2.x);
				}
				if (size.y < 0) {
					size.y = -1;
					std::swap(c1.y, c2.y);
				}
				glScissor(c1.x, c1.y, size.x, size.y);
			}
			else {
				glDisable(GL_SCISSOR_TEST);
			}

			glEnable(GL_DEPTH_TEST);
			if (p.params.depthTestMode == DepthTestMode::Disabled) {
				glDepthFunc(GL_ALWAYS);
				//glDisable(GL_DEPTH_TEST); // supposedly this cooks the depth mask?
			}
			else {
				glDepthFunc(static_cast<GLenum>(p.params.depthTestMode));
			}


			if (p.params.cullMode == FaceCulling::None) {
				glDisable(GL_CULL_FACE);
			}
			else {
				glEnable(GL_CULL_FACE);
				glCullFace(p.params.cullMode == FaceCulling::Frontface ? GL_FRONT : GL_BACK);
			}

			glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(p.params.polygonFillMode));
				
			int activeTexture = 0;
			for (auto& f : p.textures) {
				p.params.shader->Uniform(f.shaderSamplerName, activeTexture);
				f.textureToBind->Use(activeTexture);
				activeTexture++;
			}

			for (auto& renderGroup : p.thingsToDraw) {
				renderGroup->meshpool->BindVAO(p.params.shader);
				renderGroup->meshpool->indices.Bind(GL_ELEMENT_ARRAY_BUFFER);
				for (auto& c : renderGroup->commands) {
					//glPointSize(5);
					unsigned baseVertexOffset = renderGroup->meshpool->vertices.GetOffset() / renderGroup->meshpool->format.GetNonInstancedVertexSize();
					unsigned firstIndexOffset = renderGroup->meshpool->indices.GetOffset();
					unsigned instanceOffset = renderGroup->meshpool->instances.GetOffset() / renderGroup->meshpool->format.GetInstancedVertexSize();
					glDrawElementsInstancedBaseVertexBaseInstance(renderGroup->primitiveType, c.count, GL_UNSIGNED_INT, (void*)(unsigned int)((c.firstIndex + firstIndexOffset) * sizeof(unsigned int)), c.instanceCount, c.baseVertex + baseVertexOffset, c.baseInstance + instanceOffset);
				}
			}
		}
		else {
			auto& p = std::get<ProcessedComputePass>(pass);

			if (p.dependencyWriteMemoryBarrierBits != 0) {
				glMemoryBarrier(p.dependencyWriteMemoryBarrierBits);
			}
			
			if (p.setUniforms.operator bool()) p.setUniforms(p.shader);
			p.shader->Dispatch(p.workgroupSize);
		}
	}
}

void RenderGraph::AddPass(std::shared_ptr<RenderPass> pass) {
	// TODO: ATTACHMENTS/RESOURCES WITH SAME NAME MUST HAVE SAME PARAMS, OR BE SPECIFIED EXTERNALLY

	// can't add same pass twice
	Assert(drawPasses.contains(pass->name) == false && computePasses.contains(pass->name) == false);

	auto drawPass = std::dynamic_pointer_cast<DrawPass>(pass);
	auto computePass = std::dynamic_pointer_cast<ComputePass>(pass);
	Assert(drawPass || computePass);

	for (auto& t : pass->boundAttachments) {
		Assert(std::holds_alternative<std::string>(t.texture));
		if (t.willRead) {
			for (auto& d : pass->dependencies) {
				if (d == std::get<std::string>(t.texture)) goto foundOutput4;
			}
			Assert(false);
			foundOutput4:;
		}
	}

	// Verify that render target attachments are specified in pass outputs
	// TODO: buffer outputs exist
	if (drawPass) {
		if (std::holds_alternative<FramebufferRenderTargetDescriptor>(drawPass->renderTarget)) {
			auto fbt = std::get<FramebufferRenderTargetDescriptor>(drawPass->renderTarget);

			Assert(fbt.colorAttachments.size() > 0 || fbt.depthStencilAttachment.has_value());
			for (auto& attachment : fbt.colorAttachments) {
				for (auto& name : pass->outputs) {
					if (name == attachment.attachmentName) goto foundOutput;
				}
				Assert(false);
				foundOutput:;
			}
			if (fbt.depthStencilAttachment.has_value()) {
				for (auto& name : pass->outputs) {
					if (name == fbt.depthStencilAttachment->attachmentName) goto foundOutput3;
				}
				Assert(false);
				foundOutput3:;
			}

			// Validate that all attachments written to exist
			for (auto& attachment : fbt.colorAttachments) Assert(attachments.contains(attachment.attachmentName));
			if (fbt.depthStencilAttachment) Assert(attachments.contains(fbt.depthStencilAttachment->attachmentName));

			// Validate that all attachments written to are the same size.
			glm::uvec2 size;
			if (fbt.depthStencilAttachment)
				size = AttachmentSize(fbt.depthStencilAttachment->attachmentName);
			else
				size = AttachmentSize(fbt.colorAttachments[0].attachmentName);
			for (auto& attachment : fbt.colorAttachments) {
				Assert(size == AttachmentSize(attachment.attachmentName));
			}
		}
		else {
			for (auto& name : pass->outputs)
				if (name == WINDOW_RESOURCE_NAME) goto foundOutput2;
			Assert(false);
		foundOutput2:;
		}

		drawPasses[pass->name] = drawPass;
	}
	else {
		computePasses[pass->name] = computePass;
	}

	dirty = true;
}

void RenderGraph::RemovePass(std::shared_ptr<RenderPass> pass) {
	drawPasses.erase(pass->name);
	computePasses.erase(pass->name);
	dirty = true;
}

void RenderGraph::CreateAttachment(FramebufferAttachmentFormatDescriptor attachment) {
	Assert(!attachments.contains(attachment.resourceName));
	Assert(attachment.format != Texture::Auto_8Bit);


	Framebuffer::Attachment a;
	if (attachment.renderbuffer) {
		RenderbufferCreateParams rbp{
			.storageFormat = static_cast<GLenum>(attachment.format),
			.size = attachment.size,
		};
		a = std::make_shared<Renderbuffer>(rbp);
	}
	else {
		TextureCreateParams tcp({});
		tcp.format = attachment.format;
		tcp.renderBuffer = attachment.renderbuffer;
		a = std::make_shared<Texture>(attachment.size, tcp, Texture::Texture2DFlat);
	}

	AttachmentResource resource{ .name = attachment.resourceName, .hardwareResource = a, .accesses = {}, .destroy = false };
	attachments.emplace(attachment.resourceName, resource);

	dirty = true;
}

void RenderGraph::Compile() {
	Assert(dirty);
	dirty = false;
	renderOrder.clear();

	for (auto& f : framebuffers) {
		f.destroy = true;
		f.attachmentLocations.clear();
		f.readableAttachments.clear();
	}

	// Use Khan's algorithm to topologically sort our render passes into an order that ensures any node is visited only after its dependencies are.
		// This sort also minimizes OpenGL state changes by trying to keep passes with the same framebuffer/shaders/textures/etc. close together
	std::vector<std::shared_ptr<RenderPass>> output;
	{
		std::unordered_set<std::string> orphans;
		std::unordered_map<std::string, std::vector<std::string>> parentsToChildren;
		std::unordered_map<std::string, unsigned> numParents;
		for (auto& [parentName, parent] : drawPasses) {
			std::unordered_set<std::string> childrenNames;
			for (auto& outputName : parent->outputs) {
				// find nodes that require this output; those are the children
				for (auto& [potentialChildName, child] : drawPasses) {

					bool alreadyDependency = false;
					for (auto& c : parentsToChildren[parentName]) {
						if (c == potentialChildName) {
							alreadyDependency = true;
							break;
						}
					}
					if (alreadyDependency) continue;

					for (auto& dep : child->dependencies) {
						if (dep == outputName) {
							parentsToChildren[parentName].push_back(potentialChildName);
							if (!numParents.contains(potentialChildName)) numParents[potentialChildName] = 0;
							numParents[potentialChildName]++;
							break;
						}
					}
				}
			}
			bool isARoot = true;
			for (auto& d : parent->dependencies) {
				// TODO: resources available at the beginning of the frame shouldn't trigger this
				isARoot = false;
				break;
			}
			if (isARoot)
				orphans.insert(parentName);
		}

		while (orphans.size() > 0) {
			auto parentName = *orphans.begin();
			orphans.erase(orphans.begin());
			output.push_back(drawPasses[parentName]);

			std::sort(parentsToChildren[parentName].begin(), parentsToChildren[parentName].end(), [this](const std::string& passNameA, const std::string& passNameB) -> bool {
				auto& p1 = drawPasses.at(passNameA);
				auto& p2 = drawPasses.at(passNameB);
				return passNameA < passNameB; // TODO stupid way to do it

				//if (p1->params.shader != p2->params.shader) return p1->params.shader.get() < p2->params.shader.get();
				//else 
				});

			for (auto& childName : parentsToChildren[parentName]) {
				numParents[childName]--;
				if (numParents[childName] == 0) {
					numParents.erase(childName);
					orphans.insert(childName);
				}
			}
			parentsToChildren.erase(parentName);
		}

		Assert(parentsToChildren.empty() && numParents.empty()); // if this fails then a circular dependency exists
	}
	


	// Convert the DrawPasses into ProcessedRenderPasses
	int passI = 0;
	std::unordered_map<std::string, LogicalResource> logicalResources;

	while (passI < output.size()) {

		// Determine resource lifetimes so that we can determine which resources can alias the same hardware resource
		for (auto& write : output[passI]->outputs) {
			logicalResources[write].lifetime.firstWritePassIndex = std::min(logicalResources[write].lifetime.firstWritePassIndex, passI);
			logicalResources[write].lifetime.lastWritePassIndex = std::max(logicalResources[write].lifetime.lastWritePassIndex, passI);
		}
		for (auto& read : output[passI]->dependencies) {
			logicalResources[read].lifetime.firstReadPassIndex = std::min(logicalResources[read].lifetime.firstReadPassIndex, passI);
			logicalResources[read].lifetime.lastReadPassIndex = std::max(logicalResources[read].lifetime.lastReadPassIndex, passI);
		}

		auto drawPass = std::dynamic_pointer_cast<DrawPass>(output[passI]);
		auto computePass = std::dynamic_pointer_cast<ComputePass>(output[passI]);
		if (drawPass) {
			ProcessedDrawPass p(drawPass);

			if (std::holds_alternative<FramebufferRenderTargetDescriptor>(drawPass->renderTarget)) {
				auto descriptor = std::get<FramebufferRenderTargetDescriptor>(drawPass->renderTarget);
				for (auto& a : descriptor.colorAttachments) {
					Assert(logicalResources.contains(a.attachmentName));
					//logicalResources[a.attachmentName].framebufferAttachmentInfo = a;
					logicalResources[a.attachmentName].type = ResourceType::FramebufferAttachment;
					//set.writtenAttachments.push_back(a.resourceName);
				}
				if (descriptor.depthStencilAttachment) {
					Assert(logicalResources.contains(descriptor.depthStencilAttachment->attachmentName));
					logicalResources[descriptor.depthStencilAttachment->attachmentName].type = ResourceType::FramebufferAttachment;
				}
			}

			renderOrder.push_back(p);
			// TODO: combining passes with compatible textures
		}
		else {
			Assert(false);
		}
		
		passI++;
	}

	// Make sure they aren't doing anything funky with the window/default framebuffer and that they actually draw something.
	Assert(logicalResources[std::string(WINDOW_RESOURCE_NAME)].lifetime.firstReadPassIndex = INT_MAX); // You cannot read the window/default framebuffer contents.
	Assert(logicalResources[std::string(WINDOW_RESOURCE_NAME)].lifetime.firstWritePassIndex != INT_MAX);
	logicalResources[std::string(WINDOW_RESOURCE_NAME)].lifetime.firstWritePassIndex = -1;
	logicalResources[std::string(WINDOW_RESOURCE_NAME)].lifetime.lastWritePassIndex = -1;
	logicalResources[std::string(WINDOW_RESOURCE_NAME)].type = ResourceType::FramebufferAttachment; // TODO: ???

	// Add in external resources
	// TODO

	// Error on any resources that aren't written to or where reads happen before writes.
	for (auto& [_, rsrc] : logicalResources) {
		Assert(rsrc.lifetime.firstWritePassIndex != INT_MAX);
		Assert(rsrc.lifetime.lastWritePassIndex < rsrc.lifetime.firstReadPassIndex);
	}

	std::unordered_map<std::string, Texture*> attachmentLocations;

	// Setup render targets
	for (auto& p : renderOrder) {
		if (std::holds_alternative<ProcessedDrawPass>(p)) {
			auto& pass = std::get<ProcessedDrawPass>(p);
			auto& drawPass = drawPasses.at(pass.sources.at(0));
			if (std::holds_alternative<FramebufferRenderTargetDescriptor>(drawPass->renderTarget)) {
				auto& target = std::get<FramebufferRenderTargetDescriptor>(drawPass->renderTarget);

				std::vector<ResourceLifeTime> colorAttachmentLifetimes;
				for (auto& t : target.colorAttachments) {
					colorAttachmentLifetimes.push_back(logicalResources[t.attachmentName].lifetime);
				}
				FramebufferResource& framebuffer = GetFramebuffer(target);
				std::vector<GLenum> drawBuffers;
				for (auto& t : target.colorAttachments) {
					GLenum attachmentLocation = framebuffer.attachmentLocations.at(t.attachmentName);
					drawBuffers.push_back(attachmentLocation);
				}

				for (auto& [name, texture] : framebuffer.readableAttachments) {
					if (attachmentLocations.contains(name)) Assert(attachmentLocations[name] == texture);
					attachmentLocations[name] = texture;
				}

				pass.bindRenderTarget = [drawBuffers, framebuffer, target]() {
					framebuffer.hardwareResource->Bind(drawBuffers);
					for (unsigned i = 0; i < target.colorAttachments.size(); i++) {
						auto& a = target.colorAttachments[i];
						if (a.loadPolicy == AttachmentLoadPolicy::Clear) {
							glClearBufferfv(GL_COLOR, i, &a.clearColor[0]);
						}

						glBlendEquationi(i, static_cast<GLenum>(a.blendFunc));
						glBlendFunci(i, static_cast<GLenum>(a.blendingSrcFactor), static_cast<GLenum>(a.blendingDstFactor));
					}

					if (target.depthStencilAttachment) {
						glClearDepth(target.depthStencilAttachment->clearColor.x);
						glClearStencil(target.depthStencilAttachment->clearColor.y);
						glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // TODO WHAT IF THEY DONT WANNA CLEAR BOTH
					}
					};
			}
			else {
				auto target = std::get<WindowRenderTargetDescriptor>(drawPass->renderTarget);
				pass.bindRenderTarget = [target]() {
					Framebuffer::Unbind();
					if (target.loadPolicy == AttachmentLoadPolicy::Clear) {
						glClearColor(target.clearColor.r, target.clearColor.g, target.clearColor.b, target.clearColor.a);
						glClear(GL_COLOR_BUFFER_BIT);
					}
					glBlendEquation(static_cast<GLenum>(target.blendFunc));
					glBlendFunc(static_cast<GLenum>(target.blendingSrcFactor), static_cast<GLenum>(target.blendingDstFactor));
					};
			}
		}
		else {
			Assert(false);// TODO does processedcomputepass need anything?
		}
	}

	// Setup attachment texture bindings
	for (auto& p : renderOrder) {
		if (std::holds_alternative<ProcessedDrawPass>(p)) {
			auto& pass = std::get<ProcessedDrawPass>(p);
			auto& drawPass = drawPasses.at(pass.sources.at(0));
			for (auto& usageDescriptor : drawPass->boundAttachments) {
				pass.textures.push_back(TextureBinding{
					.textureToBind = attachmentLocations.at(std::get<std::string>(usageDescriptor.texture)),
					.shaderSamplerName = usageDescriptor.textureUsageLocation
					});
			}
		}
		else {
			Assert(false);
		}
	}

	// TODO buffers

	// destroy unused framebuffers
	for (unsigned i = 0; i < framebuffers.size(); i++) {
		if (framebuffers[i].destroy) {
			framebuffers[i] = framebuffers.back();
			framebuffers.pop_back();
			i--;
		}
	}
}

glm::uvec2 RenderGraph::AttachmentSize(std::string name) {
	Assert(attachments.contains(name));
	glm::uvec2 size;
	if (std::holds_alternative < std::shared_ptr<Texture>>(attachments[name].hardwareResource))
		size = std::get<std::shared_ptr<Texture>>(attachments[name].hardwareResource)->GetSize();
	else
		size = std::get<std::shared_ptr<Renderbuffer>>(attachments[name].hardwareResource)->size;
	return size;
}

bool RenderGraph::Compatible(const FramebufferRenderTargetDescriptor& requirements, const FramebufferResource& hardwareResource)
{
	return false; // TODO
}

RenderGraph::FramebufferResource& RenderGraph::GetFramebuffer(FramebufferRenderTargetDescriptor params) {
	
	// See if we already have a framebuffer with the requested attachments
	for (auto& f : framebuffers) {
		bool hasAnAttachment = false;
		for (auto& attachment : params.colorAttachments) {
			bool contains = f.attachmentLocations.contains(attachment.attachmentName);
			if (!contains && hasAnAttachment) {
				Assert(false); // TODO: APPENDING ATTACHMENTS, BUT ONLY IF 
			}
		}
	}

	// See if this framebuffer can alias an existing one and if so use that
	for (auto& f : framebuffers) {
		if (Compatible(params, f)) {
			f.destroy = false;
			return f;
		}
	}

	
	glm::uvec2 size = params.colorAttachments.empty() ? AttachmentSize(params.depthStencilAttachment->attachmentName) : AttachmentSize(params.colorAttachments[0].attachmentName);
	std::vector<Framebuffer::Attachment> colorAttachments;
	std::unordered_map<std::string, GLenum> attachmentLocations; 
	std::unordered_map<std::string, Texture*> textures;
	unsigned i = 0;
	for (auto& att : params.colorAttachments) {
		colorAttachments.push_back(attachments.at(att.attachmentName).hardwareResource);
	}
	std::optional<Framebuffer::Attachment> depthStencil;
	if (params.depthStencilAttachment.has_value()) {
		depthStencil.emplace(attachments.at(params.depthStencilAttachment->attachmentName).hardwareResource);
	}
	auto framebuffer = std::make_shared<Framebuffer>(size.x, size.y, colorAttachments, depthStencil);
	for (unsigned j = 0; j < colorAttachments.size(); j++) {
		if (std::holds_alternative<std::shared_ptr<Texture>>(framebuffer->colorAttachments[j])) {
			textures[params.colorAttachments[j].attachmentName] = std::get<std::shared_ptr<Texture>>(framebuffer->colorAttachments[j]).get();
		}
		attachmentLocations[params.colorAttachments[j].attachmentName] = GL_COLOR_ATTACHMENT0 + i;
	}
	if (framebuffer->depthAndStencilAttachment && std::holds_alternative<std::shared_ptr<Texture>>(*framebuffer->depthAndStencilAttachment)) {
		textures[params.depthStencilAttachment->attachmentName] = std::get<std::shared_ptr<Texture>>(*framebuffer->depthAndStencilAttachment).get();
	}
	if (framebuffer->depthAndStencilAttachment) attachmentLocations[params.depthStencilAttachment->attachmentName] = GL_DEPTH_STENCIL_ATTACHMENT;
	framebuffers.push_back(FramebufferResource{
		.hardwareResource = framebuffer,
		.attachmentLocations = attachmentLocations,
		.readableAttachments = textures,
		.destroy = false,
		//.size = size,
		//.isRenderbuffer = isRenderbuffer,
		//.attachmentFormats = attachmentFormats,

		});

	return framebuffers.back();
}

ProcessedDrawPass::ProcessedDrawPass(std::shared_ptr<DrawPass> drawPass) {
	params = drawPass->params;
	setUniforms = drawPass->uniformSupplier;
	thingsToDraw = drawPass->drawnObjects;
	sources.push_back(drawPass->name);
}

//ProcessedDrawPass::ProcessedDrawPass(std::shared_ptr<ComputePass> computePass) {
//	sources.push_back(computePass->name);
//	Assert(false);
//}

ProcessedComputePass::ProcessedComputePass(std::shared_ptr<ComputePass> computePass):
source(computePass->name),
workgroupSize(computePass->workgroupSize),
shader(computePass->shader),
setUniforms(computePass->uniformSupplier)
{

}

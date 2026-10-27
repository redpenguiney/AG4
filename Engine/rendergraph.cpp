#include "rendergraph.hpp"
#include "rendergroup.hpp"
#include "framebuffer.hpp"
#include "static_meshpool.hpp"
#include "GL/glew.h"
#include "shader_program.hpp"
#include "compute_shader_program.hpp"
#include <unordered_set>
#include <algorithm>
#include "buffered_buffer.hpp"
#include <climits>
#include <window.hpp>

constexpr bool ANNOTATE_FRAMES_FOR_RENDERDOC = true; // TODO: turning this on randomly breaks rendering

RenderGraph::RenderGraph() {
	connections.push_back(Window::Get().onWindowResize.Connect([this](Window*, glm::uvec2 oldSize, glm::uvec2 newSize) {
		dirty = true; // TODO: could technically add check to see if anything cares about window size. but like let's be real everything should care about window size.
	}));
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
	if (dirty) {
		DebugLogInfo("Recompiling rendergraph.")
		Compile();
	}

	if (ANNOTATE_FRAMES_FOR_RENDERDOC) glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "COMMITING HARDWARE BUFFERS");
	for (auto& buf : hardwareBuffers) {
		//if (buf->ubo) {
			buf->buf.Commit();
		//}
	}
	if (ANNOTATE_FRAMES_FOR_RENDERDOC) glPopDebugGroup();

	if (ANNOTATE_FRAMES_FOR_RENDERDOC) glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "RENDERING PROCESSED PASSES");
	for (auto& pass : renderOrder) {

		if (std::holds_alternative<ProcessedDrawPass>(pass)) {
			auto& p = std::get<ProcessedDrawPass>(pass);

			if (ANNOTATE_FRAMES_FOR_RENDERDOC) {
				std::string src = "DRAWPASS(";
				for (auto& name : p.sources) src += name + ",";
				if (!p.sources.empty()) src.pop_back();
				src += ")";
				glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, src.c_str());
			}

			if (p.dependencyWriteMemoryBarrierBits != 0) {
				glMemoryBarrier(p.dependencyWriteMemoryBarrierBits);
			}

			p.bindRenderTarget();

			bool somethingToDraw = false;
			for (auto& s : p.sources) {
				if (drawPasses.at(s)->drawnObjects.empty() == false) {
					somethingToDraw = true;
					break;
				}
			}

			if (somethingToDraw) {
				p.params.shader->Use();
				if (p.setUniforms.operator bool()) p.setUniforms(p.params.shader);

				unsigned uboI = 0;
				for (auto& uboRequested : p.params.shader->GetShaderUBOs()) {
					auto logicalBuf = logicalBuffers.at(uboRequested.name);
					logicalBuf.hardwareResource->buf.BindBase(GL_UNIFORM_BUFFER, uboI);
				}

				//std::unordered_set<int> usedBufferIndices;
				/*for (auto& buf : p.bufferBindings) {
					int bufIndex = -1;
					if (buf.locationToBindTo == GL_UNIFORM_BUFFER) {
						unsigned i = 0;
						for (auto& ubo : p.params.shader->GetShaderUBOs()) {
							if (ubo.name == buf.shaderBindingName) {
								bufIndex = i;
								break;
							}
							i++;
						}

					}
					else if (buf.locationToBindTo == GL_SHADER_STORAGE_BUFFER) {
						unsigned i = 0;
						for (auto& ssbo : p.params.shader->GetShaderSSBOs()) {
							if (ssbo.name == buf.shaderBindingName) {
								bufIndex = i;
								break;
							}
							i++;
						}
					}
					else {
						Assert(false);
					}

					Assert(bufIndex);
					buf.bufferToBind->BindBase(buf.locationToBindTo, bufIndex);
				}*/

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
					//glDisable(GL_DEPTH_TEST); // todo: supposedly this cooks the depth mask?
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

				if (p.params.stencilTestMode == DepthTestMode::Disabled) {
					glDisable(GL_STENCIL_TEST);
				}
				else {
					glEnable(GL_STENCIL_TEST);
					glStencilMask(p.params.stencilWriteMask.to_ulong());
					glStencilFunc(static_cast<GLenum>(p.params.stencilTestMode), p.params.stencilTestComparisonValue, p.params.stencilTestMask.to_ulong());
					glStencilOp(static_cast<GLenum>(p.params.stencilTestFailureAction), static_cast<GLenum>(p.params.stencilTestSuccessDepthTestFailureAction), static_cast<GLenum>(p.params.stencilTestSuccessDepthTestSuccessAction));
					//DebugLogInfo("OH NO ", p.params.stencilWriteMask, " but ", p.params.stencilTestMask, " on ", p.sources[0]);
				}

				int activeTexture = 0;
				for (auto& f : p.textures) {
					if (f.shaderSamplerName == "fontMap") {
						//DebugLogInfo("YO");
					}
					p.params.shader->Uniform(f.shaderSamplerName, activeTexture, true);
					f.textureToBind->Use(activeTexture);
					activeTexture++;
				}

				for (auto& src : p.sources) {
					if (ANNOTATE_FRAMES_FOR_RENDERDOC) glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, (std::string("RENDERGROUPS(") + src + ")").c_str());
					for (RenderGroup* renderGroup : drawPasses.at(src)->drawnObjects) {
						renderGroup->meshpool->BindVAO(p.params.shader);
						renderGroup->meshpool->indices.Bind(GL_ELEMENT_ARRAY_BUFFER);
						if (renderGroup->meshpool->boneTransforms) {
							p.params.shader->Uniform("maxBones", renderGroup->meshpool->format.GetBoneCapacity());
							renderGroup->meshpool->boneTransforms->BindBase(GL_SHADER_STORAGE_BUFFER, Meshpool::BONE_SSBO_BINDING_INDEX);
						}
						for (auto& c : renderGroup->commands) {
							//glPointSize(5);
							//if (c.count == 6) {
							//	DebugLogInfo("Sus");
							//}

							unsigned baseVertexOffset = renderGroup->meshpool->vertices.GetOffset() / renderGroup->meshpool->format.GetNonInstancedVertexSize();
							unsigned firstIndexOffset = renderGroup->meshpool->indices.GetOffset();
							unsigned instanceOffset = renderGroup->meshpool->instances.GetOffset() / renderGroup->meshpool->format.GetInstancedVertexSize();
							glDrawElementsInstancedBaseVertexBaseInstance(renderGroup->primitiveType, c.count, GL_UNSIGNED_INT, (void*)(uintptr_t)(unsigned int)((c.firstIndex + firstIndexOffset)), c.instanceCount, c.baseVertex + baseVertexOffset, c.baseInstance + instanceOffset);
						}
					}

					if (ANNOTATE_FRAMES_FOR_RENDERDOC) glPopDebugGroup();
				}

			}

			if (ANNOTATE_FRAMES_FOR_RENDERDOC) glPopDebugGroup();
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
	if (ANNOTATE_FRAMES_FOR_RENDERDOC) glPopDebugGroup();
	
	if (ANNOTATE_FRAMES_FOR_RENDERDOC) glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "FLIPPING HARDWARE BUFFERS");
	// todo: could defer this call probably
	for (auto& buf : hardwareBuffers) {
		//if (buf->ubo) {
			buf->buf.Flip();
		//}
	}

	if (ANNOTATE_FRAMES_FOR_RENDERDOC) glPopDebugGroup();
}

void RenderGraph::AddPass(std::shared_ptr<RenderPass> pass) {
	// TODO: ATTACHMENTS/RESOURCES WITH SAME NAME MUST HAVE SAME PARAMS, OR BE SPECIFIED EXTERNALLY

	// can't add same pass twice
	Assert(drawPasses.contains(pass->name) == false && computePasses.contains(pass->name) == false);

	auto drawPass = std::dynamic_pointer_cast<DrawPass>(pass);
	auto computePass = std::dynamic_pointer_cast<ComputePass>(pass);
	Assert(drawPass || computePass);

	for (auto& t : pass->boundTextures) {
		Assert(!std::holds_alternative<TextureCreateParams>(t.texture));
		if (std::holds_alternative<std::string>(t.texture)) {
			if (t.willRead) {
				for (auto& d : pass->dependencies) {
					if (d == std::get<std::string>(t.texture)) goto foundOutput4;
				}
				Assert(false);
			foundOutput4:;
			}
		}
	}

	/*for (auto& t : pass->uniformBuffers) {
		Assert(logicalBuffers.contains(t.bufferName) && logicalBuffers[t.bufferName].ubo == true);
		for (auto& d : pass->dependencies) {
			if (d == t.bufferName) goto foundOutputAgain;
		}
		Assert(false);
		foundOutputAgain:;
	}*/

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
			for (auto& attachment : fbt.colorAttachments) {
				Assert(logicalAttachments.contains(attachment.attachmentName));
				//Assert(logicalAttachments[attachment.attachmentName].hardwareResource != nullptr); won't exist until compiletime
			}
			if (fbt.depthStencilAttachment) {
				Assert(logicalAttachments.contains(fbt.depthStencilAttachment->attachmentName));
				//Assert(logicalAttachments[fbt.depthStencilAttachment->attachmentName].hardwareResource != nullptr); won't exist until compiletime 
			}

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

static Framebuffer::Attachment GetAttachmentForDescriptor(FramebufferAttachmentFormatDescriptor params) {
	Assert(params.format != Texture::Auto_8Bit);
	
	glm::uvec2 attachmentSize = params.size + glm::uvec2(glm::ceil(params.windowSize * glm::vec2(Window::Get().Size())));

	Framebuffer::Attachment a;
	if (params.renderbuffer) {
		RenderbufferCreateParams rbp{
			.storageFormat = static_cast<GLenum>(params.format),
			.size = attachmentSize,
		};
		a = std::make_shared<Renderbuffer>(rbp);
	}
	else {
		TextureCreateParams tcp({});
		tcp.format = params.format;
		tcp.wrappingBehaviour = Texture::WrapClampToEdge;
		tcp.filteringBehaviour = params.filteringBehaviour;
		tcp.wrappingBehaviour = params.wrappingBehaviour;
		tcp.renderBuffer = params.renderbuffer;
		a = std::make_shared<Texture>(attachmentSize, tcp, Texture::Texture2DFlat);
	}

	return a;
}

void RenderGraph::CreateAttachment(FramebufferAttachmentFormatDescriptor descriptor) {
	if (logicalAttachments.contains(descriptor.resourceName)) {
		if (logicalAttachments[descriptor.resourceName].descriptor == descriptor) {
			return; // no need to recompile rendergraph
		}
		else { // update existing attachment
			logicalAttachments[descriptor.resourceName].descriptor = descriptor;
			dirty = true;
		}
	}
	else {
		// declare new logical attachment
		LogicalAttachmentResource attachment;
		attachment.descriptor = descriptor;
		attachment.hardwareResource = nullptr;
		logicalAttachments.emplace(descriptor.resourceName, attachment);
	}
	//Assert(!logicalAttachments.contains(attachment.resourceName));



	

	//AttachmentResource resource{ .name = attachment.resourceName, .hardwareResource = a, .accesses = {}, .destroy = false };
	//attachments.emplace(attachment.resourceName, resource);

	//dirty = true;
}

void RenderGraph::DeclareUniformBuffer(std::string name, size_t size) {
	LogicalBufferResource rsrc;
	rsrc.ubo = true;
	rsrc.name = name;
	rsrc.requestedSize = size;
	rsrc.access.firstWritePassIndex = -1;
	rsrc.access.lastWritePassIndex = -1;
	rsrc.access.firstReadPassIndex = 0;
	rsrc.access.lastReadPassIndex = INT_MAX;

	auto hardwareRsrc = std::make_shared<BackedBufferResource>(BufferedBuffer(GL_UNIFORM_BUFFER, 1, size));
	hardwareRsrc->ubo = true;
	hardwareRsrc->accesses.push_back(rsrc.access);
	rsrc.hardwareResource = hardwareRsrc;

	logicalBuffers.emplace(name, rsrc);
 	hardwareBuffers.push_back(hardwareRsrc);
}

void RenderGraph::UploadUniformBuffer(std::string uboName, void* data, size_t len, size_t byteOffset) {
	Assert(logicalBuffers.at(uboName).ubo && logicalBuffers.at(uboName).hardwareResource);
	Assert(logicalBuffers.at(uboName).hardwareResource->buf.GetSize() >= len + byteOffset);
	memcpy(logicalBuffers.at(uboName).hardwareResource->buf.Data() + byteOffset, data, len);
}

void RenderGraph::Compile() {
	Assert(dirty);
	dirty = false;
	renderOrder.clear();

	// disconnect hardware stuff from logical stuff; it will all be reassigned during graph compilation
	for (auto& [n, l] : logicalBuffers) {
		// ubos are persistent
		if (!l.ubo) l.hardwareResource = nullptr;
	}

	for (auto& [n, l] : logicalAttachments) {
		l.hardwareResource = nullptr;
		l.firstWrite = INT_MAX;
		l.lastRead = INT_MIN;
	}

	// flag hardware stuff for destruction; things that we can't reuse will be actually destroyed at end of graph compilation.
	for (auto& f : framebuffers) {
		f.destroy = true;
		f.attachmentLocations.clear();
		f.readableAttachments.clear();
	}

	for (auto& f : hardwareBuffers) {
		if (f->ubo) continue;
		f->destroy = true;
		f->accesses.clear();
	}

	for (auto& a : hardwareAttachments) {
		//a.accesses.clear();
		a->users.clear();
		//a.destroy = true; don't need a destroy flag, destroy if no users
	}

	for (auto& [n,p] : drawPasses) {
		for (RenderGroup* group : p->drawnObjects) {
			Assert(group->commands.size() > 0);
		}
	}

	// Use Khan's algorithm to topologically sort our render passes into an order that ensures any node is visited only after its dependencies are.
		// TODO: This sort should also minimize OpenGL state changes by trying to keep passes with the same framebuffer/shaders/textures/etc. close together
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
				if (logicalBuffers.contains(d) && logicalBuffers[d].access.firstWritePassIndex == -1) {
					// then this resource is there from the start so it's okay to put this node at the head of the graph
				}
				else {
					isARoot = false;
					break;
				}
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

	for (auto& [name, buf] : logicalBuffers) {
		logicalResources[name].lifetime = buf.access;
		logicalResources[name].type = ResourceType::Buffer;
	}

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
					Assert(logicalAttachments.contains(a.attachmentName));
					//logicalResources[a.attachmentName].framebufferAttachmentInfo = a;
					logicalResources[a.attachmentName].type = ResourceType::FramebufferAttachment;
					//set.writtenAttachments.push_back(a.resourceName);

					logicalAttachments[a.attachmentName].firstWrite = std::min(passI, logicalAttachments[a.attachmentName].firstWrite);
				}
				if (descriptor.depthStencilAttachment) {
					Assert(logicalResources.contains(descriptor.depthStencilAttachment->attachmentName));
					Assert(logicalAttachments.contains(descriptor.depthStencilAttachment->attachmentName));
					logicalResources[descriptor.depthStencilAttachment->attachmentName].type = ResourceType::FramebufferAttachment;
				}
			}

			renderOrder.push_back(p);
			// TODO: combining passes with compatible textures
		}
		else {
			Assert(false);
		}

		for (auto& reqTexBinding : output[passI]->boundTextures) {
			if (std::holds_alternative<std::string>(reqTexBinding.texture)) {
				auto attachmentName = std::get<std::string>(reqTexBinding.texture);
				Assert(logicalAttachments.contains(attachmentName));
				logicalAttachments[attachmentName].lastRead = std::max(passI, logicalAttachments[attachmentName].lastRead);
			}
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

	// Assign hardware attachments
	for (auto& [attachmentName, logicalAttachment] : logicalAttachments) {

		// try to assign existing hardware attachment if a valid one exists to enable aliasing and reuse across compilations
		// TODO: overly restrictive aliasing. With judicous glViewporting/etc. a large hardware attachment could simultaneously service several smaller ones, which would obviously be useful for some stuff
		std::shared_ptr<AttachmentResource> rsrc = nullptr;
		for (auto& candidate : hardwareAttachments) {

			// must be same size/format
			if (ActualAttachmentSize(candidate->attachment) != AttachmentSize(attachmentName)) continue;
			if (candidate->format != logicalAttachment.descriptor) continue;

			// can't alias if usages overlap
			bool okay = true;
			for (auto& user : candidate->users) {
				if (user->lastRead > logicalAttachment.firstWrite && user->firstWrite < logicalAttachment.lastRead) {
					okay = false;
					break;
				}
			}
			if (!okay) continue;

			rsrc = candidate;
			break;
		}

		// otherwise make a new one
		if (!rsrc) {
			rsrc = std::make_shared<AttachmentResource>();
			rsrc->attachment = GetAttachmentForDescriptor(logicalAttachment.descriptor);
			rsrc->format = logicalAttachment.descriptor;
			hardwareAttachments.push_back(rsrc);
		}

		logicalAttachment.hardwareResource = rsrc;
		rsrc->users.push_back(&logicalAttachment);
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
							glColorMaski(i, true, true, true, true);
							glClearBufferfv(GL_COLOR, i, &a.clearColor[0]);
						}

						glBlendEquationi(i, static_cast<GLenum>(a.blendFunc));
						glBlendFunci(i, static_cast<GLenum>(a.blendingSrcFactor), static_cast<GLenum>(a.blendingDstFactor));
					}

					if (target.depthStencilAttachment && target.depthStencilAttachment->loadPolicy == AttachmentLoadPolicy::Clear) {
						glDepthMask(true);
						glStencilMask(0xFFFF);
						glClearDepth(target.depthStencilAttachment->clearColor.x);
						glClearStencil((GLint)target.depthStencilAttachment->clearColor.y);
						glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // TODO WHAT IF THEY DONT WANNA CLEAR BOTH
					}
					};
			}
			else {
				auto target = std::get<WindowRenderTargetDescriptor>(drawPass->renderTarget);
				pass.bindRenderTarget = [target]() {
					Framebuffer::Unbind();
					if (target.loadPolicy == AttachmentLoadPolicy::Clear) {
						glClearColor(target.clearColor.x, target.clearColor.y, target.clearColor.z, target.clearColor.w);
						glClear(GL_COLOR_BUFFER_BIT);
					}
					if (target.clearDepth) {
						glClearDepth(target.clearDepthValue);
						glClear(GL_DEPTH_BUFFER_BIT);
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
			for (auto& n : pass.sources) {
				auto& drawPass = drawPasses.at(n);
				for (auto& usageDescriptor : drawPass->boundTextures) {
					Texture* tex = nullptr;
					if (std::holds_alternative<std::string>(usageDescriptor.texture)) {
						tex = attachmentLocations.at(std::get<std::string>(usageDescriptor.texture));
					}
					else if (std::holds_alternative < std::shared_ptr<Texture>>(usageDescriptor.texture)) {
						tex = std::get<std::shared_ptr<Texture>>(usageDescriptor.texture).get();
					}
					else {
						Assert(false);
						//std::unreachable();
					}
					pass.textures.push_back(TextureBinding{
						.textureToBind = tex,
						.shaderSamplerName = usageDescriptor.textureUsageLocation
						});
				}
			}
			
		}
		else {
			Assert(false);
		}
	}

	// Provide hardware buffers
	for (auto& [name, l] : logicalBuffers) {
		if (l.ubo) {
			Assert(l.hardwareResource);
		}
		else {
			for (auto& candidate : hardwareBuffers) {
				for (auto& access : candidate->accesses) {
					if (access.firstWritePassIndex < l.access.lastReadPassIndex && access.lastReadPassIndex > l.access.firstWritePassIndex) {
						goto incompatible;
					}
				}

				l.hardwareResource = candidate;
				l.hardwareResource->accesses.push_back(l.access);
				l.hardwareResource->destroy = false;
				break;

				incompatible:;
			}

			if (!l.hardwareResource) { // create buffer
				auto buffer = std::make_shared<BackedBufferResource>(BufferedBuffer(GL_SHADER_STORAGE_BUFFER, 2, l.requestedSize));
				buffer->destroy = false;
				buffer->ubo = false;
				buffer->accesses.push_back(l.access);

				l.hardwareResource = buffer;
				hardwareBuffers.push_back(buffer);
			}
		}
	}

	// Setup buffer bindings for individual passes
	/*for (auto& p : renderOrder) {
		if (std::holds_alternative<ProcessedDrawPass>(p)) {
			auto& pass = std::get<ProcessedDrawPass>(p);
			for (auto& n : pass.sources) {
				auto& drawPass = drawPasses.at(n);
				for (auto& buf : drawPass->uniformBuffers) {
					Assert(logicalResources.contains(buf.bufferName));
					Assert(logicalBuffers.contains(buf.bufferName));
					Assert(logicalResources[buf.bufferName].type == ResourceType::Buffer);
					Assert(logicalBuffers[buf.bufferName].hardwareResource&& logicalBuffers[buf.bufferName].hardwareResource->ubo);
					pass.bufferBindings.push_back(BufferBinding{
						.bufferToBind = &logicalBuffers[buf.bufferName].hardwareResource->buf,
						.locationToBindTo = GL_UNIFORM_BUFFER,
						.shaderBindingName = buf.bufferName
						});
				}
			}
		}
	}*/

		// destroy unused framebuffers
	for (size_t i = 0; i < framebuffers.size(); i++) {
		if (framebuffers[i].destroy) {
			framebuffers[i] = framebuffers.back();
			framebuffers.pop_back();
			i--;
		}
	}

	// destroy unused hardware buffers
	for (size_t i = 0; i < hardwareBuffers.size(); i++) {
		if (hardwareBuffers[i]->destroy) {
			hardwareBuffers[i] = hardwareBuffers.back();
			hardwareBuffers.pop_back();
			i--;
		}
	}

	// destroy unused hardware attachments
	for (size_t i = 0; i < hardwareAttachments.size(); i++) {
		if (hardwareAttachments[i]->users.empty()) {
			hardwareAttachments[i] = hardwareAttachments.back();
			hardwareAttachments.pop_back();
			i--;
		}
	}
}

glm::uvec2 RenderGraph::AttachmentSize(std::string name) {
	Assert(logicalAttachments.contains(name));
	glm::uvec2 size = logicalAttachments[name].descriptor.size;
	size += glm::uvec2(glm::ceil(glm::vec2(Window::Get().Size()) * logicalAttachments[name].descriptor.windowSize));
	return size;
}

glm::uvec2 RenderGraph::ActualAttachmentSize(const Framebuffer::Attachment& attachment) {
	if (std::holds_alternative<std::shared_ptr<Texture>>(attachment)) {
		return std::get<std::shared_ptr<Texture>>(attachment)->GetSize();
	}
	else {
		return std::get<std::shared_ptr<Renderbuffer>>(attachment)->size;
	}
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
		colorAttachments.push_back(logicalAttachments.at(att.attachmentName).hardwareResource->attachment);
	}
	std::optional<Framebuffer::Attachment> depthStencil;
	if (params.depthStencilAttachment.has_value()) {
		depthStencil.emplace(logicalAttachments.at(params.depthStencilAttachment->attachmentName).hardwareResource->attachment);
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
	//thingsToDraw = drawPass->drawnObjects;
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

RenderGraph::BackedBufferResource::BackedBufferResource(BufferedBuffer buf): buf(std::move(buf)) {
}

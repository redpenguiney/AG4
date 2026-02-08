#include "rendergraph.hpp"
#include "rendergroup.hpp"
#include "framebuffer.hpp"
#include "static_meshpool.hpp"
#include "GL/glew.h"
#include "shader_program.hpp"
#include <unordered_set>

RenderGraph::RenderGraph() {
	
}

static void BindRenderTarget(WindowRenderTargetDescriptor target) {
	Framebuffer::Unbind();
	if (target.loadPolicy == AttachmentLoadPolicy::Clear) {
		glClearColor(target.clearColor.r, target.clearColor.g, target.clearColor.b, target.clearColor.a);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	glBlendFunc(static_cast<GLenum>(target.blendingSrcFactor), static_cast<GLenum>(target.blendingDstFactor));
}

static void BindRenderTarget(FramebufferRenderTargetDescriptor target) {
	Assert(false);
	//std::vector<glm::vec4> clearValues;
	//clearValues.resize(p.renderTarget.framebuffer->textureAttachments.size(), { -1, -1, -1, -1 });
	//for (auto& policy : p.renderTarget.loadPolicies) {
		//policy.
	//}
	//p.renderTarget.framebuffer->Clear(clearValues);
}


void RenderGraph::Render() {
	if (dirty) Compile();

	for (auto& set : renderSets) {
		for (auto& p : set.passes) {
			std::visit([](auto&& x) {BindRenderTarget(x); }, p.renderTarget);

			p.params.shader->Use();

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

			if (p.params.depthTestMode == DepthTestMode::Disabled) {
				glDisable(GL_DEPTH_TEST);
			}
			else {
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(static_cast<GLenum>(p.params.depthTestMode));
			}


			if (p.params.cullMode == FaceCulling::None) {
				glDisable(GL_CULL_FACE);
			}
			else {
				glEnable(GL_CULL_FACE);
				glCullFace(p.params.cullMode == FaceCulling::Frontface ? GL_FRONT : GL_BACK);
			}

			for (auto& renderGroup : p.thingsToDraw) {
				renderGroup->meshpool->BindVAO(p.params.shader);
				renderGroup->meshpool->indices.Bind();
				for (auto& c : renderGroup->commands) {
					//glPointSize(5);
					unsigned baseVertexOffset = renderGroup->meshpool->vertices.GetOffset() / renderGroup->meshpool->format.GetNonInstancedVertexSize();
					unsigned firstIndexOffset = renderGroup->meshpool->indices.GetOffset();
					unsigned instanceOffset = renderGroup->meshpool->instances.GetOffset() / renderGroup->meshpool->format.GetInstancedVertexSize();
					glDrawElementsInstancedBaseVertexBaseInstance(renderGroup->primitiveType, c.count, GL_UNSIGNED_INT, (void*)(unsigned int)((c.firstIndex + firstIndexOffset) * sizeof(unsigned int)), c.instanceCount, c.baseVertex + baseVertexOffset, c.baseInstance + instanceOffset);
				}
			}
		}
	}
}

void RenderGraph::AddPass(std::shared_ptr<RenderPass> pass) {
	// can't add same pass twice
	Assert(passes.contains(pass->name) == false);

	auto drawPass = std::dynamic_pointer_cast<DrawPass>(pass);
	auto computePass = std::dynamic_pointer_cast<ComputePass>(pass);
	Assert(drawPass || computePass);

	// Verify that render target attachments are specified in pass outputs
	if (drawPass) {
		if (std::holds_alternative<FramebufferRenderTargetDescriptor>(drawPass->renderTarget)) {
			for (auto& attachment : std::get<FramebufferRenderTargetDescriptor>(drawPass->renderTarget).attachments) {
				for (auto& name : pass->outputs) {
					if (name == attachment.resourceName) goto foundOutput;
				}
				Assert(false);
				foundOutput:;
			}
		}
		else {
			for (auto& name : pass->outputs)
				if (name == WINDOW_RESOURCE_NAME) goto foundOutput2;
			Assert(false);
		foundOutput2:;
		}
	}

	passes[pass->name] = pass;
	dirty = true;
}

void RenderGraph::RemovePass(std::shared_ptr<RenderPass> pass) {
	passes.erase(pass->name);
	dirty = true;
}

void RenderGraph::Compile() {
	Assert(dirty);
	dirty = false;
	renderSets.clear();

	// Use Khan's algorithm to topologically sort our render passes into an order that ensures any node is visited only after its dependencies are.
	std::vector<std::shared_ptr<RenderPass>> output;
	{
		std::unordered_set<std::string> orphans;
		std::unordered_map<std::string, std::vector<std::string>> parentsToChildren;
		std::unordered_map<std::string, unsigned> numParents;
		for (auto& [parentName, parent] : passes) {
			std::unordered_set<std::string> childrenNames;
			for (auto& outputName : parent->outputs) {
				// find nodes that require this output; those are the children
				for (auto& [potentialChildName, child] : passes) {

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
			output.push_back(passes[parentName]);

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
	
	// Assign render passes into as few RenderSets as possible
	unsigned passI = 0;

	while (passI < output.size()) {
		RenderSet set;
		auto drawPass = std::dynamic_pointer_cast<DrawPass>(output[passI]);
		auto computePass = std::dynamic_pointer_cast<ComputePass>(output[passI]);
		ProcessedRenderPass p = drawPass ? ProcessedRenderPass(drawPass) : ProcessedRenderPass(computePass);
		set.passes.push_back(p);
		set.source.push_back(output[passI]);
		renderSets.push_back(set);
		// TODO: combining passes with compatible textures, combining sets with no resource conflicts, optimizing intra-set pass order
		passI++;
	}

	// Determine resource lifetimes so that we can determine which resources can alias the same hardware resource
	std::unordered_map<std::string, LogicalResource> logicalResources;

	for (int i = 0; i < renderSets.size(); i++) {
		for (auto& pass : renderSets[i].source) {
			for (auto& write : pass->outputs) {
				logicalResources[write].lifetime.firstWritePassIndex = std::min(logicalResources[write].lifetime.firstWritePassIndex, i);
				logicalResources[write].lifetime.lastWritePassIndex = std::max(logicalResources[write].lifetime.lastWritePassIndex, i);
			}
			for (auto& read : pass->dependencies) {
				logicalResources[read].lifetime.firstReadPassIndex = std::min(logicalResources[read].lifetime.firstReadPassIndex, i);
				logicalResources[read].lifetime.lastReadPassIndex = std::max(logicalResources[read].lifetime.lastReadPassIndex, i);
			}
			
		}
	}

	// Make sure they aren't doing anything funky with the window/default framebuffer and that they actually draw something.
	Assert(logicalResources[std::string(WINDOW_RESOURCE_NAME)].lifetime.firstReadPassIndex = INT_MAX); // You cannot read the window/default framebuffer contents.
	Assert(logicalResources[std::string(WINDOW_RESOURCE_NAME)].lifetime.firstWritePassIndex != INT_MAX);
	logicalResources[std::string(WINDOW_RESOURCE_NAME)].lifetime.firstWritePassIndex = -1;
	logicalResources[std::string(WINDOW_RESOURCE_NAME)].lifetime.lastWritePassIndex = -1;

	// Add in external resources
	// TODO

	// Error on any resources that aren't written to or where reads happen before writes.
	for (auto& [_, rsrc] : logicalResources) {
		Assert(rsrc.lifetime.firstWritePassIndex != INT_MAX);
		Assert(rsrc.lifetime.lastWritePassIndex < rsrc.lifetime.firstReadPassIndex);
	}

	// Assign/validate framebuffer/attachments according to the following:
		// All attachments written by a pass must be the same size.
		// The last write of all the attachments in a framebufer must occur in a RenderSet before the first read of said attachments.
	for (auto& set : renderSets) {
		std::vector<FramebufferAttachmentDescriptor> currentFramebufferAttachments;
		for (auto& pass : set.passes) {
			if (std::holds_alternative<FramebufferRenderTargetDescriptor>(pass.renderTarget)) {
				auto& attachments = std::get<FramebufferRenderTargetDescriptor>(pass.renderTarget).attachments;
				for (auto& att : attachments) {
					if (currentFramebufferAttachments.size() > 0) {
						Assert(att.size == currentFramebufferAttachments.back().size);
					}
					currentFramebufferAttachments.push_back(att);
				}
			}
		}
	}

	// Allocate hardware resources to each logical resource
	// TODO optimize
}

//RenderGraph::FramebufferResource& RenderGraph::GetFramebuffer(FramebufferRenderTargetDescriptor params) {
//	
//}

ProcessedRenderPass::ProcessedRenderPass(std::shared_ptr<DrawPass> drawPass) {
	params = drawPass->params;
	renderTarget = drawPass->renderTarget;
	thingsToDraw = drawPass->drawnObjects;
	sources.push_back(drawPass->name);
}

ProcessedRenderPass::ProcessedRenderPass(std::shared_ptr<ComputePass> computePass) {
	sources.push_back(computePass->name);
	Assert(false);
}

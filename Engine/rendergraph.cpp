#include "rendergraph.hpp"
#include "rendergroup.hpp"
#include "framebuffer.hpp"
#include "static_meshpool.hpp"
#include "GL/glew.h"
#include "shader_program.hpp"

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
	for (auto& p : renderPasses) {
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
		glDepthFunc(static_cast<GLenum>(p.params.depthTestMode));


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
				glPointSize(5);
				unsigned baseVertexOffset = renderGroup->meshpool->vertices.GetOffset() / renderGroup->meshpool->format.GetNonInstancedVertexSize();
				unsigned firstIndexOffset = renderGroup->meshpool->indices.GetOffset();
				unsigned instanceOffset = renderGroup->meshpool->instances.GetOffset() / renderGroup->meshpool->format.GetInstancedVertexSize();
				glDrawElementsInstancedBaseVertexBaseInstance(renderGroup->primitiveType, c.count, GL_UNSIGNED_INT, (void*)(unsigned int)((c.firstIndex + firstIndexOffset) * sizeof(unsigned int)), c.instanceCount, c.baseVertex + baseVertexOffset, c.baseInstance + instanceOffset);
			}
			
		}
	}
}

void RenderGraph::AddPass(std::shared_ptr<RenderPass> pass) {
	// can't add same pass twice
	for (auto& p : renderPasses) {
		for (auto& n: p.sources)
		Assert(n != pass);
	}

	// Ensure this render pass runs after all of its dependencies do and before any dependencies that require it
	size_t minOrder = 0;
	size_t maxOrder = 1000000000000000000;
	Assert(pass->framebufferDependencies.empty()); // TODO
	for (auto& p : renderPasses) {
		for (auto& src : p.sources) {
			for (auto& dependencyName : pass->dependencies) {
				if (src->name == dependencyName) {
					minOrder = p.order + 1;
				}
			}

			for (auto& depenencyName : src->dependencies) {

			}
		}
	}

	if (auto drawPass = std::dynamic_pointer_cast<DrawPass>(pass)) renderPasses.emplace_back(drawPass);
	else if (auto computePass = std::dynamic_pointer_cast<ComputePass>(pass)) renderPasses.emplace_back(computePass);
	else Assert(false);

	renderPasses.back().order = 0;
}

void RenderGraph::RemovePass(std::shared_ptr<RenderPass> pass) {
	for (auto it = renderPasses.begin(); it != renderPasses.end(); it++) {
		for (unsigned i = 0; i < it->sources.size(); i++) {
			if (pass->name == it->sources[i]) {
				it->sources[i] = it->sources.back();
				it->sources.pop_back();
				if (it->sources.empty()) {
					renderPasses.erase(it);
				}
			}
		}
	}
}

ProcessedRenderPass::ProcessedRenderPass(std::shared_ptr<DrawPass> drawPass) {
	params = drawPass->params;
	renderTarget = drawPass->renderTarget;
	thingsToDraw = drawPass->drawnObjects;
	sources.push_back(drawPass->name);
}

ProcessedRenderPass::ProcessedRenderPass(std::shared_ptr<ComputePass> computePass) {
	sources.push_back(computePass->name);
}

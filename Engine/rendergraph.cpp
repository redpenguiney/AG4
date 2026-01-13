#include "rendergraph.hpp"
#include "rendergroup.hpp"
#include "framebuffer.hpp"
#include "static_meshpool.hpp"
#include "GL/glew.h"
#include "shader_program.hpp"

RenderGraph::RenderGraph(std::vector<std::shared_ptr<RenderPass>> passes) {
	for (auto& p : passes) {
		ProcessedRenderPass pass;
		pass.params = p->params;
		pass.renderTarget = p->renderTarget;
		pass.thingsToDraw = p->drawnObjects;


	}
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
		std::visit(&BindRenderTarget, p.renderTarget);

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

		for (auto& renderGroup : p.thingsToDraw) {
			renderGroup->meshpool->BindVAO(p.params.shader);
			renderGroup->meshpool->indices.Bind();
			for (auto& c : renderGroup->commands) {
				unsigned baseVertexOffset = renderGroup->meshpool->vertices.GetOffset() / renderGroup->meshpool->format.GetNonInstancedVertexSize();
				unsigned firstIndexOffset = renderGroup->meshpool->indices.GetOffset();
				unsigned instanceOffset = renderGroup->meshpool->instances.GetOffset() / renderGroup->meshpool->format.GetInstancedVertexSize();
				glDrawElementsInstancedBaseVertexBaseInstance(renderGroup->primitiveType, c.count, GL_UNSIGNED_INT, (void*)(unsigned int)((c.firstIndex + firstIndexOffset) * sizeof(unsigned int)), c.instanceCount, c.baseVertex + baseVertexOffset, c.baseInstance + instanceOffset);
			}
			
		}
	}
}

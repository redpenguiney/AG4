#include "rendergraph.hpp"
#include "rendergroup.hpp"
#include "framebuffer.hpp"
#include "static_meshpool.hpp"
#include "GL/glew.h"

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

}

void RenderGraph::Render() {
	for (auto& p : renderPasses) {
		std::visit(&BindRenderTarget, p.renderTarget);
		p.renderTarget.framebuffer->Bind();
		std::vector<glm::vec4> clearValues;
		clearValues.resize(p.renderTarget.framebuffer->textureAttachments.size(), { -1, -1, -1, -1 });
		for (auto& policy : p.renderTarget.loadPolicies) {
			policy.
		}
		p.renderTarget.framebuffer->Clear(clearValues);
	}
}

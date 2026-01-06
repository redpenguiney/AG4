#include "rendergraph.hpp"
#include "rendergroup.hpp"

RenderGraph::RenderGraph(std::vector<std::shared_ptr<RenderPass>> passes) {
	for (auto& p : passes) {
		ProcessedRenderPass pass;
		pass.params = p->params;
		pass.renderTarget = p->renderTarget;
		pass.thingsToDraw = p->drawnObjects;


	}
}

void RenderGraph::Render() {
	for (auto& p : renderPasses) {
		p.
	}
}

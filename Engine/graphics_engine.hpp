#pragma once
#include <vector>
#include "rendergraph_node.hpp"
#include "rendergraph.hpp"
#include <memory>

class GraphicsEngine {
public:
	static GraphicsEngine& Get();

	void RenderScene(double dt);

	void UpdateRenderGraph(std::vector<std::shared_ptr<RendergraphNode>> nodes);

private:
	RenderGraph activeRenderGraph;

	GraphicsEngine();
	~GraphicsEngine();
};
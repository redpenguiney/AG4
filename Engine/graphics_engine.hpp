#pragma once
#include <vector>
#include "rendergraph_node.hpp"
#include "rendergraph.hpp"
#include <memory>
#include "camera.hpp"

class StaticMeshpool;

class GraphicsEngine {
public:
	Camera currentCamera;

	static GraphicsEngine& Get();

	void RenderScene(double dt);

	void UpdateRenderGraph(std::vector<std::shared_ptr<RenderPass>> nodes);

private:
	void WriteModelMatrices();

	std::shared_ptr<RenderGraph> activeRenderGraph;

	// Meshpools for static geometry. Exactly one meshpool per unique vertex format.
	std::vector<std::shared_ptr<StaticMeshpool>> staticMeshpools;

	GraphicsEngine();
	~GraphicsEngine();
};
#pragma once
#include <vector>
#include "rendergraph.hpp"
#include <memory>
#include "camera.hpp"

class RenderPass;

class StaticMeshpool;

class GraphicsEngine {
public:
	Camera currentCamera;

	static GraphicsEngine& Get();

	void RenderScene(double dt);

	void UpdateRenderGraph(std::vector<std::shared_ptr<RenderPass>> nodes);

	// A solid default for gameobjects. 
	std::shared_ptr<RenderPass> mainDrawingPass;

private:
	void WriteModelMatrices();

	std::shared_ptr<RenderGraph> activeRenderGraph;

	// Meshpools for static geometry. Exactly one meshpool per unique vertex format.
	std::vector<std::shared_ptr<StaticMeshpool>> staticMeshpools;

	GraphicsEngine();
	~GraphicsEngine();
};
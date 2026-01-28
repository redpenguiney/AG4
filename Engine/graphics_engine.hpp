#pragma once
#include <vector>
#include "rendergraph.hpp"
#include <memory>
#include "camera.hpp"

class RenderPass;

class StaticMeshpool;

class GraphicsEngine {
public:
	// Change at your discretion.
	Camera currentCamera;

	static GraphicsEngine& Get();

	void RenderScene(double dt);

	void AddRenderPass(std::shared_ptr<RenderPass> node);
	void RemoveRenderPass(std::shared_ptr<RenderPass> node);

	// Default renderpasses for newly created GameobjectCreateParams. Will have a reasonable default, but change at your discretion. 
	std::vector<std::shared_ptr<DrawPass>> defaultDrawingPasses;

private:
	void WriteModelMatrices();

	std::shared_ptr<RenderGraph> renderGraph;

	// Meshpools for static geometry. Exactly one meshpool per unique vertex format.
	std::vector<std::shared_ptr<StaticMeshpool>> staticMeshpools;

	GraphicsEngine();
	~GraphicsEngine();
};
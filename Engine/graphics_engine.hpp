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

	// (DrawPasses are automatically added when a gameobject using one is created for the first ime, and removed dynamically)
	void AddComputePass(std::shared_ptr<ComputePass> node);
	void RemoveComputePass(std::shared_ptr<ComputePass> node);

	void AddAttachment(FramebufferAttachmentFormatDescriptor attachment);

	// Default renderpasses for newly created GameobjectCreateParams. Will have a reasonable default, but change at your discretion. 
	std::vector<std::shared_ptr<DrawPass>> defaultDrawingPasses;

private:
	// RenderGroup dynamically adds/removes drawpasses to/from renderGraph
	friend class RenderGroup;

	void WriteModelMatrices();

	std::shared_ptr<RenderGraph> renderGraph;

	// Meshpools for static geometry. Exactly one meshpool per unique vertex format.
	std::vector<std::shared_ptr<StaticMeshpool>> staticMeshpools;

	GraphicsEngine();
	~GraphicsEngine();
};
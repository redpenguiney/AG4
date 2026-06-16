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

	void AddUniformBuffer(std::string name, size_t size);
	void DeleteUniformBuffer(std::string name);
	void UploadToUniformBuffer(std::string name, void* data, size_t len, size_t destOffset);

	void AddAttachment(FramebufferAttachmentFormatDescriptor attachment);
	void ForceAddDrawPass(std::shared_ptr<DrawPass> pass);

	// Default renderpasses for newly created GameobjectCreateParams. Will have a reasonable default, but change at your discretion. 
	std::vector<std::shared_ptr<DrawPass>> defaultDrawingPasses;

private:
	// RenderGroup dynamically adds/removes drawpasses to/from renderGraph
	friend class RenderGroup;

	void WriteModelMatrices();
	void WriteBones();

	std::shared_ptr<RenderGraph> renderGraph;

	// Meshpools for static geometry. Exactly one meshpool per unique vertex format.
	std::vector<std::shared_ptr<StaticMeshpool>> staticMeshpools;

	GraphicsEngine();
	~GraphicsEngine();
};
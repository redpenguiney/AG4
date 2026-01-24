#pragma once
#include "buffered_buffer.hpp"
#include "indirect_draw_command.hpp"
#include "rendergraph_node.hpp"
#include <string>

class Meshpool;

struct TextureBinding {
	std::shared_ptr<Texture> textureToBind;
	std::string shaderSamplerName;
};

struct CommandSet {
	unsigned firstCommand;
	unsigned nCommands;
};

// The conceptual difference between this and RenderPass is:
// - RenderPass only describes what the user WANTS to happen on that drawing step (what textures they want to use).
// - ProcessedRenderPass contains the information needed to actually make that occur (how those textures are actually stored in texture arrays, the actual framebuffer object being used).
struct ProcessedRenderPass {
	std::vector<std::shared_ptr<RenderPass>> sources;
	size_t order;

	RenderTargetDescriptor renderTarget;
	std::vector<TextureBinding> textures;
	std::vector<RenderGroup*> thingsToDraw;
	
	RenderingParameters params;

	ProcessedRenderPass(std::shared_ptr<DrawPass> drawPass);
	ProcessedRenderPass(std::shared_ptr<ComputePass> computePass);
};

// A compiled render graph. The user should not work with this class directly.
class RenderGraph {
public:
	RenderGraph();
	void Render();

	void AddPass(std::shared_ptr<RenderPass> pass);
	void RemovePass(std::shared_ptr<RenderPass> pass);

	~RenderGraph() = default;
	RenderGraph(const RenderGraph&) = delete;
private:

	//BufferedBuffer indirectDrawingCommandBuffer;

	// Already sorted.
	std::vector<ProcessedRenderPass> renderPasses;
};
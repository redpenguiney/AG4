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

struct DrawingSet {
	GLenum primitiveType;
	std::vector<CommandSet> commands;
	std::shared_ptr<Meshpool> meshpool;
};

// The conceptual difference between this and RenderPass is:
// - RenderPass only describes what the user WANTS to happen on that drawing step (what textures they want to use).
// - ProcessedRenderPass contains the information needed to actually make that occur (how those textures are actually stored in texture arrays, the actual framebuffer object being used).
struct ProcessedRenderPass {
	size_t order;

	RenderTargetDescriptor renderTarget;
	std::vector<TextureBinding> textures;
	std::vector<RenderGroup*> thingsToDraw;
	
	RenderingParameters params;

};

// A compiled render graph. The user should not work with this class directly.
class RenderGraph {
public:
	RenderGraph(std::vector<std::shared_ptr<RenderPass>> passes);
	void Render();

	~RenderGraph() = default;
	RenderGraph(const RenderGraph&) = delete;
private:
	//BufferedBuffer indirectDrawingCommandBuffer;

	// Already sorted.
	std::vector<ProcessedRenderPass> renderPasses;
};
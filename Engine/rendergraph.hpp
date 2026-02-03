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
// - RenderPass only describes what the user WANTS to happen on that drawing step (like what textures they want to use).
// - ProcessedRenderPass contains the information needed to actually make that occur (like how those textures are actually stored in texture arrays, the actual framebuffer object being used).
struct ProcessedRenderPass {
	std::vector<std::string> sources;
	size_t order;

	RenderTargetDescriptor renderTarget;
	std::vector<TextureBinding> textures;
	std::vector<RenderGroup*> thingsToDraw;
	
	RenderingParameters params;

	ProcessedRenderPass(std::shared_ptr<DrawPass> drawPass);
	ProcessedRenderPass(std::shared_ptr<ComputePass> computePass);
};

struct RenderSet {
	// these passes can be carried out in any order.
	std::vector<ProcessedRenderPass> passes;

	// TODO: barriers/synchroninzation info

	// sorts passes so that they are in the order which minimizes OpenGL state changes
	void OptimizePassOrder();
};

struct ResourceLifeTime {
	// indices are with respect to renderSets
	// -1 for first/lastwritePassIndex indicates that it has a value from the very start.
	int firstWritePassIndex = INT_MAX;
	int lastWritePassIndex = INT_MIN;

	int firstReadPassIndex = INT_MAX;
	int lastReadPassIndex = INT_MIN;

	/*void Verify() {
		if (!isStatic) {
			Assert(firstWritePassIndex <= lastWritePassIndex);
			Assert(lastWritePassIndex < firstReadPassIndex);
		}
		Assert(firstReadPassIndex <= lastReadPassIndex);
	}*/
};

enum class ResourceType {
	FramebufferAttachment,
	//ShaderStorageBuffer,
};

struct LogicalResource {
	ResourceLifeTime lifetime;
	
	std::variant<FramebufferAttachmentDescriptor> framebufferAttachmentInfo;
};

// A compiled render graph. The user should not work with this class directly.
class RenderGraph {
public:
	RenderGraph();
	void Render();

	void AddPass(std::shared_ptr<RenderPass> pass);
	void RemovePass(std::shared_ptr<RenderPass> pass);

	void AddLogicalResource(std::string name, FramebufferAttachmentDescriptor renderTarget);
	void RemoveLogicalResource(std::string name);

	~RenderGraph() = default;
	RenderGraph(const RenderGraph&) = delete;
private:
	bool dirty = true;
	// updates the contents of renderSets
	void Compile();

	//BufferedBuffer indirectDrawingCommandBuffer;
	std::unordered_map<std::string, std::shared_ptr<RenderPass>> passes;

	std::unordered_map<std::string, std::shared_ptr<Framebuffer>> framebuffers;

	// Ordered based on the dependency graph of the RenderPasses in each RenderSet
	std::vector<RenderSet> renderSets;
};
#pragma once
#include "buffered_buffer.hpp"
#include "indirect_draw_command.hpp"
#include "rendergraph_node.hpp"
#include <string>
#include <functional>
#include "framebuffer.hpp"

class BaseShaderProgram;
class Meshpool;

struct TextureBinding {
	Texture* textureToBind; // non-owning
	std::string shaderSamplerName;
};

struct CommandSet {
	unsigned firstCommand;
	unsigned nCommands;
};

// The conceptual difference between this and DrawPass is:
// - RenderPass only describes what the user WANTS to happen on that drawing step (like what textures they want to use).
// - ProcessedRenderPass contains the information needed to actually make that occur (like how those textures are actually stored in texture arrays, the actual framebuffer object being used).
struct ProcessedDrawPass {
	std::vector<std::string> sources;
	//size_t order;

	std::function<void()> bindRenderTarget;
	std::function<void(std::shared_ptr<BaseShaderProgram>)> setUniforms; // may be empty
	std::vector<TextureBinding> textures;
	std::vector<RenderGroup*> thingsToDraw;
	
	RenderingParameters params;

	GLbitfield dependencyWriteMemoryBarrierBits = 0;

	ProcessedDrawPass(std::shared_ptr<DrawPass> drawPass);
};

struct ProcessedComputePass {
	std::string source;
	std::shared_ptr<ComputeShaderProgram> shader;
	glm::uvec3 workgroupSize;
	std::function<void(std::shared_ptr<BaseShaderProgram>)> setUniforms; // may be empty
	GLbitfield dependencyWriteMemoryBarrierBits = 0;

	ProcessedComputePass(std::shared_ptr<ComputePass> computePass);
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
	Abstract
};

struct LogicalResource {
	ResourceLifeTime lifetime;
	ResourceType type = ResourceType::Abstract;
	//FramebufferAttachmentFormatDescriptor framebufferAttachmentInfo;
};

// A compiled render graph. The user should not work with this class directly.
class RenderGraph {
public:
	RenderGraph();
	void Render();

	void AddPass(std::shared_ptr<RenderPass> pass);
	void RemovePass(std::shared_ptr<RenderPass> pass);

	void CreateAttachment(FramebufferAttachmentFormatDescriptor attachment);

	~RenderGraph() = default;
	RenderGraph(const RenderGraph&) = delete;
private:
	bool dirty = true;
	// updates the contents of renderSets
	void Compile();

	glm::uvec2 AttachmentSize(std::string name);

	struct FramebufferResource {
		std::shared_ptr<Framebuffer> hardwareResource;
		std::unordered_map<std::string, GLenum> attachmentLocations;
		std::unordered_map<std::string, Texture*> readableAttachments;
		bool destroy;
	};

	struct AttachmentResource {
		std::string name;
		Framebuffer::Attachment hardwareResource;
		std::vector<ResourceLifeTime> accesses;
		bool destroy;
	};

	bool Compatible(const FramebufferRenderTargetDescriptor& requirements, const FramebufferResource& resource);

	FramebufferResource& GetFramebuffer(FramebufferRenderTargetDescriptor params);

	//BufferedBuffer indirectDrawingCommandBuffer;
	std::unordered_map<std::string, std::shared_ptr<DrawPass>> drawPasses;
	std::unordered_map<std::string, std::shared_ptr<ComputePass>> computePasses;

	std::vector<FramebufferResource> framebuffers;
	std::unordered_map<std::string, AttachmentResource> attachments;

	// Ordered based on the dependency graph of the RenderPasses in each RenderSet, and then in an order intended to improve performance by reducing OpenGL state changes
	std::vector<std::variant<ProcessedDrawPass, ProcessedComputePass>> renderOrder;
};
#pragma once
#include "buffered_buffer.hpp"
#include "indirect_draw_command.hpp"
#include "rendergraph_node.hpp"
#include <string>
#include <functional>
#include "framebuffer.hpp"
#include <event.hpp>

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

struct AutoArrayTexture {

};

//struct BufferBinding {
//	BufferedBuffer* bufferToBind;
//	GLenum locationToBindTo;
//	std::string shaderBindingName;
//};

// The conceptual difference between this and DrawPass is:
// - RenderPass only describes what the user WANTS to happen on that drawing step (like what textures they want to use).
// - ProcessedRenderPass contains the information needed to actually make that occur (like how those textures are actually stored in texture arrays, the actual framebuffer object being used).
struct ProcessedDrawPass {
	std::vector<std::string> sources;
	//size_t order;

	std::function<void()> bindRenderTarget;
	std::function<void(std::shared_ptr<BaseShaderProgram>)> setUniforms; // may be empty
	std::vector<TextureBinding> textures;
	//std::vector<RenderGroup*> thingsToDraw; 
	
	DrawingParameters params;

	GLbitfield dependencyWriteMemoryBarrierBits = 0;
	//std::vector<BufferBinding> bufferBindings;

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
	Buffer,
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

	// if attachment.name is for an attachment which already exists, the existing attachment descriptor will be replaced with this one.
	void CreateAttachment(FramebufferAttachmentFormatDescriptor attachment);

	void DeclareUniformBuffer(std::string name, size_t size);

	// Uploads read-only data to shader uniform block.
	// Block must have already been declared.
	// uboName refers to the name of the buffer-backed uniform block in GLSL.
	void UploadUniformBuffer(std::string uboName, void* data, size_t len, size_t byteOffset = 0);

	// Like UploadUniformBuffer(), but data may not be preserved after the next frame (so that the memory can be aliased or something idk).
	// Call every frame prior to rendering.
	// void StreamUniformBuffer(std::string uboName, void* data, size_t len);

	~RenderGraph() = default;
	RenderGraph(const RenderGraph&) = delete;
private:
	bool dirty = true;
	// updates the contents of renderSets
	void Compile();

	glm::uvec2 AttachmentSize(std::string name);

	// 1. renderpasses state that they want to read/write an attachment, specified by name
	// 2. AddAttachment() is called to associate that name with a requested format (the logical attachment).
	// 3. AddAttachment() may be called subsequent times to modify the format associated with a name.
	// 4. The render graph associates each logical attachment with an actual Framebuffer::Attachment object (the hardware attachment).
	// 5. At render graph compilation:
		// a. All preexisting hardware attachments are marked for removal.
		// b. Accesses of each logical attachment are recorded.
		// c. Each logical attachment is assigned a hardware attachment.
			// If a preexisting hardware attachment of compatible format exists...
			// ...and no other logical attachment assigned to it has conflicting accesses...
			// Use it.
			// Otherwise, generate a new hardware attachment.
		// d. delete unused hardware attachments.
		// e. do framebuffer generation

	struct FramebufferResource {
		std::shared_ptr<Framebuffer> hardwareResource;
		std::unordered_map<std::string, GLenum> attachmentLocations;
		std::unordered_map<std::string, Texture*> readableAttachments;
		bool destroy;
	};

	struct LogicalAttachmentResource;

	struct AttachmentResource {
		Framebuffer::Attachment attachment;
		std::vector<LogicalAttachmentResource*> users; // non-owning/owned by corresponding LogicalAttachmentResources
		FramebufferAttachmentFormatDescriptor format;
	};

	
	struct BackedBufferResource {
		BackedBufferResource(BufferedBuffer buf);
		std::vector<ResourceLifeTime> accesses;
		BufferedBuffer buf;
		bool ubo;
		bool destroy;
	};

	struct LogicalBufferResource {
		std::string name;

		size_t requestedSize;
		// may be nullptr
		std::shared_ptr<BackedBufferResource> hardwareResource;
		ResourceLifeTime access;
		bool ubo;
	};

	struct LogicalAttachmentResource {
		//std::string name; name is in descriptor object
		FramebufferAttachmentFormatDescriptor descriptor;

		// used to decide whether two LogicalAttachmentResources can alias the same hardwareResource
		int firstWrite, lastRead;

		std::shared_ptr<AttachmentResource> hardwareResource;
	};

	glm::uvec2 ActualAttachmentSize(const Framebuffer::Attachment& attachment);

	bool Compatible(const FramebufferRenderTargetDescriptor& requirements, const FramebufferResource& resource);

	FramebufferResource& GetFramebuffer(FramebufferRenderTargetDescriptor params);

	std::unordered_map<std::string, LogicalAttachmentResource> logicalAttachments;

	std::unordered_map<std::string, LogicalBufferResource> logicalBuffers;
	std::vector<std::shared_ptr<BackedBufferResource>> hardwareBuffers;

	//BufferedBuffer indirectDrawingCommandBuffer;
	std::unordered_map<std::string, std::shared_ptr<DrawPass>> drawPasses;
	std::unordered_map<std::string, std::shared_ptr<ComputePass>> computePasses;

	std::vector<FramebufferResource> framebuffers;
	std::vector<std::shared_ptr<AttachmentResource>> hardwareAttachments;

	// Ordered based on the dependency graph of the RenderPasses in each RenderSet, and then in an order intended to improve performance by reducing OpenGL state changes
	std::vector<std::variant<ProcessedDrawPass, ProcessedComputePass>> renderOrder;

	std::vector<Connection> connections;
};
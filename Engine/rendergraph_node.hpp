#pragma once
#include <memory>
#include <vector>
#include "glm/vec4.hpp"
#include "glm/vec2.hpp"
#include "GL/glew.h"
#include "texture_collection.hpp"

class Framebuffer;
class BufferedBuffer;
class ShaderProgram;
class ComputeShaderProgram;
class RenderGroup;
class Texture;
struct TextureCreateParams;

enum class ImageFormat : GLenum {
	RGBA_16_DECIMAL = GL_RGBA16, // google opengl "normalized integer".
	RGBA_16_INT = GL_RGBA16I,
	RGBA_16_UINT = GL_RGBA16UI,
	RGBA_32_FLOAT = GL_RGBA32F,

	DEPTH_16_DECIMAL = GL_DEPTH_COMPONENT16,
	DEPTH_24_DECIMAL = GL_DEPTH_COMPONENT24,
	DEPTH_32_DECIMAL = GL_DEPTH_COMPONENT32,
	DEPTH_32_FLOAT = GL_DEPTH_COMPONENT32F
};

enum class DepthTestMode : GLenum {
	Disabled = GL_ALWAYS, // values in depth buffer are ignored for rendering purposes.
	Less = GL_LESS,
	LEqual = GL_LEQUAL // Use this for normal stuff.
};

// See https://learnopengl.com/Advanced-OpenGL/Blending
enum class BlendFactorMode : GLenum {
	Zero = GL_ZERO,
	One = GL_ONE,
	SrcColor = GL_SRC_COLOR,
	SrcAlpha = GL_SRC_ALPHA,
	OneMinusSrcColor = GL_ONE_MINUS_SRC_COLOR,
	OneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA,
	DstColor = GL_DST_COLOR,
	DstAlpha = GL_DST_ALPHA,
	OneMinusDstColor = GL_ONE_MINUS_DST_COLOR,
	OneMinusDstAlpha = GL_ONE_MINUS_DST_ALPHA,
};

enum class BlendingEquation : GLenum {
	Addition = GL_FUNC_ADD,
	SourceMinusDest = GL_FUNC_SUBTRACT,
	DestMinusSource = GL_FUNC_REVERSE_SUBTRACT,
	Minimum = GL_MIN,
	Maximum = GL_MAX
};

// Describes what the contents of a render target should be for a render pass before we draw/write to it.
enum class AttachmentLoadPolicy {
	Load, // use the previous contents of the render target
	Clear, // 
	DontCare, // all contents of the render target will be overwritten by this pass so it doesn't matter
};

struct Attachment {
	std::string name;
	AttachmentLoadPolicy loadPolicy = AttachmentLoadPolicy::Load;
	glm::vec4 clearColor = { 0, 0, 0, 0 }; // used if loadPolicy == Clear
	ImageFormat format;

	BlendFactorMode blendingSrcFactor = BlendFactorMode::SrcAlpha;
	BlendFactorMode blendingDstFactor = BlendFactorMode::OneMinusSrcAlpha;
};

struct FramebufferRenderTargetDescriptor {
public:
	glm::uvec2 size;
	std::vector<Attachment> attachments; // used if loadPolicy == Clear
};

struct WindowRenderTargetDescriptor {
	AttachmentLoadPolicy loadPolicy = AttachmentLoadPolicy::Clear;
	glm::vec4 clearColor = { 0, 0, 0, 0 }; // used if loadPolicy == Clear
	bool clearDepth = true;

	BlendFactorMode blendingSrcFactor = BlendFactorMode::SrcAlpha;
	BlendFactorMode blendingDstFactor = BlendFactorMode::OneMinusSrcAlpha;
	BlendingEquation blendFunc = BlendingEquation::Addition;
};

using RenderTargetDescriptor = std::variant<FramebufferRenderTargetDescriptor, WindowRenderTargetDescriptor>;

struct BufferUsageDescriptor {
public:
	std::string bindTo;
	std::shared_ptr<BufferedBuffer> buffer;

	bool willRead = true;
	bool willWrite = false;
};

using TextureHandle = std::variant<std::shared_ptr<Texture>, TextureCreateParams>;

struct TextureUsageDescriptor {
public:
	std::shared_ptr<TextureHandle> texture;
	std::string textureUsageLocation; // name of the shader's sample uniform variable where this shader should be bound.

	bool willRead = true;
	bool willWrite = false;
};

enum class FaceCulling {
	Backface,
	Frontface,
	None
};

struct RenderingParameters {
	std::shared_ptr<ShaderProgram> shader;

	bool blending;

	FaceCulling cullMode = FaceCulling::Backface;

	DepthTestMode depthTestMode = DepthTestMode::LEqual;
	bool writeDepthBuffer = true; // (depth mask)

	// Scissor test
	bool scissoringEnabled = false;
	glm::ivec2 scissorCorner1;
	glm::ivec2 scissorCorner2;
};

class RenderPass {
public:
	std::string name = "UNNAMED";
	std::vector<std::string> dependencies; // names of RenderPasses that this RenderPass uses the output of
	std::vector<std::shared_ptr<Framebuffer>> framebufferDependencies; // this RenderPass won't run until all (other) RenderPasses that write to this framebuffer occur

	std::vector<BufferUsageDescriptor> buffersUsed;
	std::vector<TextureUsageDescriptor> texturesUsed;

	virtual ~RenderPass() = default;

protected:
	RenderPass() = default;
};

class DrawPass : public RenderPass {
public:
	RenderTargetDescriptor renderTarget;

	std::vector<RenderGroup*> drawnObjects; // managed by RenderGroups, non-owning

	RenderingParameters params;
};

class ComputePass : public RenderPass {
	std::shared_ptr<ComputeShaderProgram> shader;
	glm::uvec3 workgroupSize;
};
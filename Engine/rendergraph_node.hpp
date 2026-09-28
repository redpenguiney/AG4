#pragma once
#include <memory>
#include <vector>
#include <functional>
#include "glm/vec4.hpp"
#include "glm/vec2.hpp"
#include "GL/glew.h"
#include "texture_collection.hpp"

class BaseShaderProgram;
class Framebuffer;
class BufferedBuffer;
class ShaderProgram;
class ComputeShaderProgram;
class RenderGroup;
class Texture;
struct TextureCreateParams;

//enum class ImageFormat : GLenum {
//	RGBA_16_DECIMAL = GL_RGBA16, // google opengl "normalized integer".
//	RGBA_16_INT = GL_RGBA16I,
//	RGBA_16_UINT = GL_RGBA16UI,
//	RGBA_32_FLOAT = GL_RGBA32F,
//
//	DEPTH_16_DECIMAL = GL_DEPTH_COMPONENT16,
//	DEPTH_24_DECIMAL = GL_DEPTH_COMPONENT24,
//	DEPTH_32_DECIMAL = GL_DEPTH_COMPONENT32,
//	DEPTH_32_FLOAT = GL_DEPTH_COMPONENT32F
//};

// (also used for stencil test.)
enum class DepthTestMode : GLenum {
	Disabled = 0, // values in depth buffer are ignored for rendering purposes. default for stencil test
	Less = GL_LESS,
	LEqual = GL_LEQUAL, // Use this for normal stuff.
	Equal = GL_EQUAL,
	AlwaysFail = GL_NEVER,
	AlwaysPass = GL_ALWAYS // test always passes, but different from Disabled because stencil operations will run. Don't use on depth buffer, probably.
};

// how you modify stencil buffer
enum class StencilOperation : GLenum {
	Keep = GL_KEEP, // do nothing to stencil buffer
	Zero = GL_ZERO, // write zero to stencil buffer
	Set = GL_REPLACE, // write DrawingParameters::stencilTestComparisonValue to stencil buffer
	IncrementClamped = GL_INCR, // increments stencil buffer by 1, clamps at max value
	IncrementWrapping = GL_INCR_WRAP,
	DecrementClamped = GL_DECR, // decrements stencil buffer by 1, clamps at 0
	DecrementWrapping = GL_DECR_WRAP,
	BitwiseInvert = GL_INVERT
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

// A resource is written to (or has a static value), and then it is read from by other passes.
	// No write, then read, then write again or anything. 
struct Resource {
	std::string name;
	// if true, this resource can be written to.
	bool validOutput = true;

	bool saveForNextFrame = false;

	virtual ~Resource() = default;
};

// Describes what the contents of a render target should be for a render pass before we draw/write to it.
enum class AttachmentLoadPolicy {
	Load, // use the previous contents of the render target
	Clear, // 
	DontCare, // all contents of the render target will be overwritten by this pass so it doesn't matter
};

// TODO: TEXTURE SAMPLING PARAMETERS?
struct FramebufferAttachmentFormatDescriptor {
	std::string resourceName;
	// probably only set to true for depth buffers you don't want to read from inside shaders
	bool renderbuffer = false;
	//// when drawing, the shader-specified index of the output variable which should be output onto this attachment. 
	//// -1 if not drawing to this attachment. (TODO why would you want that?)
	//int drawBuffer = -1; 

	Texture::TextureFormat format = Texture::RGBA_16Float;
	glm::uvec2 size = {128, 128};
	
	// adds the window size multiplied by the given vec2 and rounded up to whatever the requested framebuffer size is. 
	// recommended when you want to ensure that framebuffer is same size as window.
	// Will dynamically update the attachment whenever window size changes.
	glm::vec2 windowSize = { 0, 0 };

	bool operator==(const FramebufferAttachmentFormatDescriptor&) const = default;
};

struct FramebufferAttachmentUsageDescriptor {
	std::string attachmentName;

	AttachmentLoadPolicy loadPolicy = AttachmentLoadPolicy::Load;

	// used if loadPolicy == Clear. x value is used for clearing depth, y value is used for clearing stencil.
	// PROTIP: clear depth to 1, not 0
	glm::vec4 clearColor = { 0, 0, 0, 0 }; 
	BlendFactorMode blendingSrcFactor = BlendFactorMode::SrcAlpha;
	BlendFactorMode blendingDstFactor = BlendFactorMode::OneMinusSrcAlpha;
	BlendingEquation blendFunc = BlendingEquation::Addition;
};

struct FramebufferRenderTargetDescriptor {
	// Note: don't include attachments this pass doesn't draw to. 
	// Corresponds 1:1 with drawbuffers.
	std::vector<FramebufferAttachmentUsageDescriptor> colorAttachments;
	std::optional<FramebufferAttachmentUsageDescriptor> depthStencilAttachment;
};

constexpr inline const char* WINDOW_RESOURCE_NAME = "__WINDOW__";

struct WindowRenderTargetDescriptor {
	AttachmentLoadPolicy loadPolicy = AttachmentLoadPolicy::Clear;
	glm::vec4 clearColor = { 0, 0, 0, 0 }; // used if loadPolicy == Clear
	bool clearDepth = true;
	float clearDepthValue = 1.0f;

	BlendFactorMode blendingSrcFactor = BlendFactorMode::SrcAlpha;
	BlendFactorMode blendingDstFactor = BlendFactorMode::OneMinusSrcAlpha;
	BlendingEquation blendFunc = BlendingEquation::Addition;
};

using RenderTargetDescriptor = std::variant<FramebufferRenderTargetDescriptor, WindowRenderTargetDescriptor>;

//struct UniformBufferUsageDescriptor {
//public:
//	// name of the GLSL variable
//	std::string bindTo;
//
//	// name of the resource
//	std::string bufferName;
//};

// if a string, then it better be the name of an attachment.
// if TextureCreateParams, then TODO add support for that first.
using TextureHandle = std::variant<std::string, std::shared_ptr<Texture>, TextureCreateParams>;

struct TextureUsageDescriptor {
public:
	TextureHandle texture;
	std::string textureUsageLocation; // name of the shader's sample uniform variable where this shader should be bound.

	bool willRead = true;
	//bool willWrite = false;
};

enum class FaceCulling {
	Backface,
	Frontface,
	None
};

enum class PolygonFillMode {
	Fill = GL_FILL,
	Lines = GL_LINE // wireframe
};

struct DrawingParameters {
	std::shared_ptr<ShaderProgram> shader;

	bool blending = false;

	FaceCulling cullMode = FaceCulling::Backface;

	DepthTestMode depthTestMode = DepthTestMode::LEqual;
	bool writeDepthBuffer = true; // (depth mask)
	PolygonFillMode polygonFillMode = PolygonFillMode::Fill;

	DepthTestMode stencilTestMode = DepthTestMode::Disabled;
	GLint stencilTestComparisonValue = 0; // value stencil buffer contents is compared to (based on specified test mode) to decide whether to pass the test
	GLuint stencilTestMask = 0xFFFF; // ANDed with with comparison value and stencil buffer value before performing test

	StencilOperation stencilTestFailureAction = StencilOperation::Keep;
	StencilOperation stencilTestSuccessDepthTestFailureAction = StencilOperation::Keep;

	// also executed when no depth testing occurs
	StencilOperation stencilTestSuccessDepthTestSuccessAction = StencilOperation::Keep;

	// all 1s = stencil writes unaffected, all 0s = attempts to write stencil buffer do nothing.
	GLuint stencilWriteMask = 0xFFFF;

	// Scissor test
	bool scissoringEnabled = false;
	glm::ivec2 scissorCorner1;
	glm::ivec2 scissorCorner2;
};

//using ShaderUniformType = std::variant<glm::mat4x4, glm::vec4, glm::vec3, glm::vec2, float, bool, unsigned, int>;

class RenderPass {
public:
	std::string name;

	std::vector<std::string> dependencies; // names of Resources that this RenderPass reads from
	std::vector<std::string> outputs; // names of Resources that this RenderPass writes to

	std::vector<TextureUsageDescriptor> boundTextures;

	// uniforms (constant globals) to be fed to shaders
	//std::vector<std::pair<std::string, ShaderUniformType>> uniforms;

	//std::vector<UniformBufferUsageDescriptor> uniformBuffers;

	// optional user defined function which if supplied can be used to set shader uniforms (constant globals) prior to drawing/compute work.
	// Passed argument is the shader about to be used.
	std::function<void(std::shared_ptr<BaseShaderProgram>)> uniformSupplier;

	virtual ~RenderPass() = default;

protected:
	RenderPass() = default;
};

class DrawPass : public RenderPass {
public:
	DrawPass() = default;
	virtual ~DrawPass();

	// Make sure you rename the pass, can't have two with the same name.
	static std::shared_ptr<DrawPass> FromTemplate(const DrawPass& other);

	// If FramebufferRenderTargetDescriptor:
		// Remember to update outputs to contain ALL attachments of this renderTarget (not just the ones it writes to).
	// If WindowRenderTargetDescriptor:
		// Remember to update outputs to contain WINDOW_RESOURCE_NAME.
	RenderTargetDescriptor renderTarget;

	std::vector<RenderGroup*> drawnObjects; // managed by RenderGroups, non-owning

	DrawingParameters params;
private:
	DrawPass(const DrawPass& other) = default;
};

class ComputePass : public RenderPass {
public:
	std::shared_ptr<ComputeShaderProgram> shader;
	glm::uvec3 workgroupSize;
};
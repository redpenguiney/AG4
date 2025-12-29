#pragma once
#include <memory>
#include <vector>
#include "glm/vec4.hpp"
#include "GL/glew.h"
#include "texture_collection.hpp"
class Material;
class Framebuffer;
class BufferedBuffer;
class ShaderProgram;

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
	Disabled = 0,
	Addition = GL_FUNC_ADD,
	SourceMinusDest = GL_FUNC_SUBTRACT,
	DestMinusSource = GL_FUNC_REVERSE_SUBTRACT,
	Minimum = GL_MIN,
	Maximum = GL_MAX
};

// Describes what the contents of a render target should be for a render pass before we draw/write to it.
enum class RenderTargetAttachmentLoadPolicy {
	Load, // use the previous contents of the render target
	Clear, // 
	DontCare, // all contents of the render target will be overwritten by this pass so it doesn't matter
};

struct AttachmentLoadPolicy {
	RenderTargetAttachmentLoadPolicy loadPolicy = RenderTargetAttachmentLoadPolicy::Load;
	glm::vec4 clearColor = { 0, 0, 0, 0 }; // used if loadPolicy == Clear
	GLenum attachmentLocation;

	BlendFactorMode blendingSrcFactor = BlendFactorMode::SrcAlpha;
	BlendFactorMode blendingDstFactor = BlendFactorMode::OneMinusSrcAlpha;
};

struct RenderTargetDescriptor {
public:
	std::vector<AttachmentLoadPolicy> loadPolicies; // used if loadPolicy == Clear

	std::shared_ptr<Framebuffer> framebuffer; // nullptr if writing directly to window contents
};

struct BufferUsageDescriptor {
public:
	GLenum bufferType;
	unsigned bufferingBindingIndex;
	//bool write = false;
	//bool read = true;
	std::shared_ptr<BufferedBuffer> buffer;
};

struct TextureUsageDescriptor {
public:

};


class RenderPass {
	std::string name;
	std::vector<std::string> dependencies; // names of RenderPasses that this RenderPass uses the output of
	std::vector<std::shared_ptr<Framebuffer>> framebufferDependencies; // this RenderPass won't run until all (other) RenderPasses that write to this framebuffer occur

	// Everything with these materials will get drawn (in no particular order, if any order at all).
	std::vector<std::shared_ptr<Material>> materials;

	std::vector<RenderTargetDescriptor> renderTargets;
	std::vector<BufferUsageDescriptor> buffersUsed;
	std::shared_ptr<TextureCollection> texturesUsed;
	std::vector<TextureUsageDescriptor> auxillaryTexturesUsed.

	std::shared_ptr<ShaderProgram> shader;
	BlendingEquation blendFunc = BlendingEquation::Disabled;

	DepthTestMode depthTestMode = DepthTestMode::LEqual;
	bool writeDepthBuffer = true; // (depth mask)

	// Scissor test
	bool scissoringEnabled = false;
	glm::ivec2 scissorCorner1;
	glm::ivec2 scissorCorner2;

	
};
#pragma once
#include "texture.hpp"
#include <glm/ext/vector_uint2.hpp>

struct RenderbufferCreateParams {
	GLenum storageFormat;
	glm::uvec2 size;
};

class Framebuffer;

class Renderbuffer {
public:
	Renderbuffer(RenderbufferCreateParams params);
	Renderbuffer(const Renderbuffer&) = delete;
	Renderbuffer(Renderbuffer&&) noexcept;
	~Renderbuffer();

	void AttachToFramebuffer(Framebuffer& attachTo, GLenum attachmentPoint);

	const GLenum format;
	const glm::uvec2 size;
private:
	GLuint renderbufferName = 0;
	static inline GLuint currentlyBoundRenderbuffer = 0;
	void Bind();
};
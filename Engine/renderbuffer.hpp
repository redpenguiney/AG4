#pragma once
#include "texture.hpp"
#include <glm/ext/vector_uint2.hpp>

struct RenderbufferCreateParams {
	GLenum storageFormat;
	GLenum attachmentPoint;
	glm::uvec2 size;
};

class Framebuffer;

class Renderbuffer {
public:
	Renderbuffer(RenderbufferCreateParams params, Framebuffer& attachTo);
	Renderbuffer(const Renderbuffer&) = delete;
	Renderbuffer(Renderbuffer&&) noexcept;
	~Renderbuffer();

private:
	GLuint renderbufferName = 0;
	static inline GLuint currentlyBoundRenderbuffer = 0;
	void Bind();
};
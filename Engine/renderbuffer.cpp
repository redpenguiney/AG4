#include "renderbuffer.hpp"
#include "framebuffer.hpp"

Renderbuffer::Renderbuffer(RenderbufferCreateParams params):
	format(params.storageFormat),
	size(params.size)
{
	glGenRenderbuffers(1, &renderbufferName);
	Bind();
	glRenderbufferStorage(GL_RENDERBUFFER, params.storageFormat, params.size.x, params.size.y);
	
}

Renderbuffer::Renderbuffer(Renderbuffer&& old) noexcept : format(old.format), size(old.size) {
	this->renderbufferName = old.renderbufferName;
	old.renderbufferName = 0;
}

Renderbuffer::~Renderbuffer() {
	if (renderbufferName != 0) {
		if (currentlyBoundRenderbuffer == renderbufferName) currentlyBoundRenderbuffer = 0;
		glDeleteRenderbuffers(1, &renderbufferName);
	}
}

void Renderbuffer::AttachToFramebuffer(Framebuffer& attachTo, GLenum attachmentPoint) {
	attachTo.Bind({});
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachmentPoint, GL_RENDERBUFFER, renderbufferName);
}

void Renderbuffer::Bind() {
	if (currentlyBoundRenderbuffer != renderbufferName) {
		currentlyBoundRenderbuffer = renderbufferName;
		glBindRenderbuffer(GL_RENDERBUFFER, renderbufferName);
	}
}

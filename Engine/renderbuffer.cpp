#include "renderbuffer.hpp"
#include "framebuffer.hpp"

Renderbuffer::Renderbuffer(RenderbufferCreateParams params, Framebuffer& attachTo)
{
	glGenRenderbuffers(1, &renderbufferName);
	Bind();
	glRenderbufferStorage(GL_RENDERBUFFER, params.storageFormat, params.size.x, params.size.y);
	attachTo.Bind({});
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, params.attachmentPoint, GL_RENDERBUFFER, renderbufferName);
}

Renderbuffer::Renderbuffer(Renderbuffer&& old) noexcept {
	this->renderbufferName = old.renderbufferName;
	old.renderbufferName = 0;
}

Renderbuffer::~Renderbuffer() {
	if (renderbufferName != 0) {
		if (currentlyBoundRenderbuffer == renderbufferName) currentlyBoundRenderbuffer = 0;
		glDeleteRenderbuffers(1, &renderbufferName);
	}
}

void Renderbuffer::Bind() {
	if (currentlyBoundRenderbuffer != renderbufferName) {
		currentlyBoundRenderbuffer = renderbufferName;
		glBindRenderbuffer(GL_RENDERBUFFER, renderbufferName);
	}
}

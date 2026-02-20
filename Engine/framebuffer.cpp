#include "framebuffer.hpp"
#include "assert.hpp"
//#include "graphics_engine.hpp"
#include "window.hpp"

Framebuffer::Framebuffer(const unsigned int fbWidth, const unsigned int fbHeight, const std::vector<Attachment>& attachments, std::optional<Attachment> depthAndStencilAttachment):
width(fbWidth), 
height(fbHeight),
bindingLocation(GL_FRAMEBUFFER),
colorAttachments(attachments),
depthAndStencilAttachment(depthAndStencilAttachment)
{
    // create openGL framebuffer
    glGenFramebuffers(1, &glFramebufferId);

    // bind it so we can set it up
    Bind({});

    GLenum attachmentI = 0;
    for (auto& c : colorAttachments) {
        if (std::holds_alternative<std::shared_ptr<Texture>>(c)) {
            auto t = std::get<std::shared_ptr<Texture>>(c);
            Assert(t->format != Texture::DEPTH24_STENCIL8);
            t->AttachToFramebuffer(*this, GL_COLOR_ATTACHMENT0 + attachmentI);
        }
        else {
            auto r = std::get<std::shared_ptr<Renderbuffer>>(c);
            Assert(r->format != Texture::DEPTH24_STENCIL8);
            r->AttachToFramebuffer(*this, GL_COLOR_ATTACHMENT0 + attachmentI);
        }
    }
    if (depthAndStencilAttachment) {
        if (std::holds_alternative<std::shared_ptr<Texture>>(*depthAndStencilAttachment)) {
            auto t = std::get<std::shared_ptr<Texture>>(*depthAndStencilAttachment);
            Assert(t->format == Texture::DEPTH24_STENCIL8);
            t->AttachToFramebuffer(*this, GL_DEPTH_STENCIL_ATTACHMENT);
        }
        else {
            auto r = std::get<std::shared_ptr<Renderbuffer>>(*depthAndStencilAttachment);
            Assert(r->format == Texture::DEPTH24_STENCIL8);
            r->AttachToFramebuffer(*this, GL_DEPTH_STENCIL_ATTACHMENT);
        }
    }

    //// attach textures (TODO: support for texture arrays, cubemaps?)
    //for (auto & params: attachmentParams) {
    //    Assert(params.format != Texture::Auto_8Bit && params.format != Texture::DEPTH24_STENCIL8);
    //    if (params.renderBuffer) {
    //        RenderbufferCreateParams rbp{
    //            .storageFormat = static_cast<GLenum>(params.format), 
    //            .attachmentPoint = GL_COLOR_ATTACHMENT0 + attachmentI,
    //            .size = {fbWidth, fbHeight},
    //        };
    //        colorAttachments.push_back(Renderbuffer(rbp, *this));
    //    }
    //    else {
    //        colorAttachments.push_back(Texture(*this, params, Texture::Texture2DFlat, GL_COLOR_ATTACHMENT0 + attachmentI));
    //    }
    //    attachmentI++;
    //}

    //if (depthAndStencilAttachmentParams) {
    //    if (depthAndStencilAttachmentParams->renderBuffer) {
    //        RenderbufferCreateParams rbp{
    //            .storageFormat = static_cast<GLenum>(depthAndStencilAttachmentParams->format),
    //            .attachmentPoint = GL_DEPTH_STENCIL_ATTACHMENT,
    //            .size = {fbWidth, fbHeight},
    //        };
    //        depthAndStencilAttachment.emplace(Renderbuffer(rbp, *this));
    //    }
    //    else {
    //        depthAndStencilAttachment.emplace(Texture(*this, *depthAndStencilAttachmentParams, Texture::Texture2DFlat, GL_DEPTH_STENCIL_ATTACHMENT));
    //    }
    //}
    
    // make sure framebuffer is valid
    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    Assert(status == GL_FRAMEBUFFER_COMPLETE);
}

Framebuffer::~Framebuffer() {
    if (currentlyBound == glFramebufferId) {
        currentlyBound = 0;
    }
    glDeleteFramebuffers(1, &glFramebufferId);
}

// TODO: apparently sometimes you want to use an argument besides GL_FRAMEBUFFER?
//void Framebuffer::Bind() {
//    std::vector<size_t> attachments;
//    for (size_t i = 0; i < attachmentNames.size(); i++) attachments.push_back(i);
//    Bind(attachments);
//}

void Framebuffer::Bind(std::vector<GLenum> attachments) {

    //std::vector<GLenum> attachments;
    //for (auto i : attachmentIndices) {
    //    attachments.push_back(attachmentNames.at(i));
    //}

    if (currentlyBound == glFramebufferId) {
        if (attachments != currentDrawBuffers) {
            glDrawBuffers((GLsizei)attachments.size(), attachments.data());
            
        }
    }
    else {
        //DebugLogInfo("Bound framebuffer. (prev was ", currentlyBound, " now ", glFramebufferId, ")");
        currentlyBound = glFramebufferId;
        glBindFramebuffer(bindingLocation, glFramebufferId);
        glViewport(0, 0, width, height);
        glDrawBuffers((GLsizei)attachments.size(), attachments.data());
    }
    currentDrawBuffers = attachments;
    
    //const GLenum buffers[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    
    
}

void Framebuffer::ClearColor(std::vector<glm::vec4> clearColors)
{
    //Assert(clearColors.size() == colorAttachmentNames.size());
    Assert(clearColors.size() == currentDrawBuffers.size());
    Assert(currentlyBound == glFramebufferId);

    for (int i = 0; i < clearColors.size(); i++) {
        if (clearColors[i] == glm::vec4(-1, -1, -1, -1)) continue;
        //if (textureAttachments[i].format == Texture::TextureFormat::RGBA_16Float) {
        glClearBufferfv(GL_COLOR, i, &clearColors[i][0]);
        //}
        //else {
            //glm::ivec4 casted =
        //}
    }
}

void Framebuffer::ClearDepth(float value) {
    Assert(currentlyBound == glFramebufferId);
    glClearDepth(value);
}

void Framebuffer::ClearStencil(int value) {
    Assert(currentlyBound == glFramebufferId);
    glClearStencil(value);
}

void Framebuffer::Unbind() {
    currentlyBound = 0;
    unsigned int windowWidth = Window::Get().width;
    unsigned int windowHeight = Window::Get().height;
    glViewport(0, 0, windowWidth, windowHeight);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // TODO: other binding locations
    glDrawBuffer(GL_FRONT_AND_BACK);
}
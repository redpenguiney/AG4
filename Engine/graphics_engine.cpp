#include "graphics_engine.hpp"
#include "memory_pool.hpp"
#include "gameobject.hpp"
#include "static_meshpool.hpp"
#include <glm/gtc/matrix_inverse.hpp>
#include "rendergraph_node.hpp"
#include "shader_program.hpp"
#include "window.hpp"
#include "mesh.hpp"
#include "rendergroup.hpp"

GraphicsEngine& GraphicsEngine::Get() {
    static GraphicsEngine GE;
    return GE;
}

void GraphicsEngine::RenderScene(double dt) {


    WriteModelMatrices();
    auto camera = currentCamera.GetCamera();
    auto cameraNoFloatingOrigin = currentCamera.GetCamera();
    cameraNoFloatingOrigin[3] = glm::vec4(-currentCamera.position, 1);
    auto proj = currentCamera.GetProj(Window::Get().Aspect());
    auto orthro = glm::ortho<float>(0, Window::Get().width, 0, Window::Get().height, -1000.0f, 1000.0f);
    ShaderProgram::SetCameraUniforms(proj * camera, proj * cameraNoFloatingOrigin, orthro); 

    Meshpool::PrepareDraw();

    if (renderGraph)
        renderGraph->Render();

    glFlush();
    Meshpool::PrepareWrite();
}

void GraphicsEngine::AddComputePass(std::shared_ptr<ComputePass> node) {
    renderGraph->AddPass(node);
}

void GraphicsEngine::RemoveComputePass(std::shared_ptr<ComputePass> node) {
    renderGraph->RemovePass(node);
}

void GraphicsEngine::AddUniformBuffer(std::string name, size_t size) {
    renderGraph->DeclareUniformBuffer(name, size);
}

void GraphicsEngine::DeleteUniformBuffer(std::string name) {
    Assert(false); // TODO
}

void GraphicsEngine::UploadToUniformBuffer(std::string name, void* data, size_t len, size_t destOffset) {
    renderGraph->UploadUniformBuffer(name, data, len, destOffset);
}

void GraphicsEngine::AddAttachment(FramebufferAttachmentFormatDescriptor attachment) {
    renderGraph->CreateAttachment(attachment);
}

void GraphicsEngine::ForceAddDrawPass(std::shared_ptr<DrawPass> pass) {
    if (!RenderGroup::drawPassNumUsers.contains(pass.get())) {
        renderGraph->AddPass(pass);
        RenderGroup::drawPassNumUsers[pass.get()] = 0;
    }
    RenderGroup::drawPassNumUsers[pass.get()]++;
}

void GraphicsEngine::WriteModelMatrices() {
    auto write = [&](auto iterable) {
        for (auto& page : iterable) {
            for (unsigned i = 0; i < POOL_OBJECTS_PER_PAGE; i++) {
                Gameobject& obj = page[i].obj;
                if (!obj.Live()) continue;

                
                glm::vec3 relPos;
                if (obj.meshpool->format.IsFloatingOriginEnabled()) {
                    relPos = obj.Position() - currentCamera.position;
                }
                else {
                    relPos = obj.Position();
                }
                glm::mat4x4 modelMatrix = glm::mat4x4(obj.GetRotSclMatrix());
                modelMatrix[3].x = relPos.x;
                modelMatrix[3].y = relPos.y;
                modelMatrix[3].z = relPos.z;
                modelMatrix[3].w = 1; // TODO is this one neccesary?
                obj.meshpool->StreamModelMatrix(obj.drawInstanceIndex, modelMatrix);
                if (obj.normalMatDirty) {
                    //obj.normalMatDirty = false; TODO: it's always dirty because we're streaming it;
                    obj.meshpool->StreamNormalMatrix(obj.drawInstanceIndex, glm::inverseTranspose(obj.GetRotSclMatrix())); // TODO: many things won't be nonuniformly scaled and this will be an unneccesary performance cost for them
                }
            }
        }
        };

    write(Gameobject::Pool::Get().GetIterable());
    write(Physobject::Pool::Get().GetIterable());
}

GraphicsEngine::GraphicsEngine() {
    defaultDrawingPasses = {};  
    renderGraph = std::shared_ptr<RenderGraph>(new RenderGraph());  
}

GraphicsEngine::~GraphicsEngine() {

}

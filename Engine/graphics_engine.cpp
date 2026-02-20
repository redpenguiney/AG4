#include "graphics_engine.hpp"
#include "memory_pool.hpp"
#include "gameobject.hpp"
#include "static_meshpool.hpp"
#include <glm/gtc/matrix_inverse.hpp>
#include "rendergraph_node.hpp"
#include "shader_program.hpp"
#include "window.hpp"
#include "mesh.hpp"

GraphicsEngine& GraphicsEngine::Get() {
    static GraphicsEngine GE;
    return GE;
}

void GraphicsEngine::RenderScene(double dt) {


    WriteModelMatrices();
    auto camera = currentCamera.GetCamera();
    auto cameraNoFloatingOrigin = camera;
    cameraNoFloatingOrigin[3] = glm::vec4(-currentCamera.position, 1);
    auto proj = currentCamera.GetProj(Window::Get().Aspect());
    ShaderProgram::SetCameraUniforms(proj * camera, proj * cameraNoFloatingOrigin, glm::identity<glm::mat4x4>()); // TODO: orthro???

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

void GraphicsEngine::AddAttachment(FramebufferAttachmentFormatDescriptor attachment) {
    renderGraph->CreateAttachment(attachment);
}

void GraphicsEngine::WriteModelMatrices() {
    for (auto& page : MemoryPool<Gameobject, GameobjectCreateParams>::Get().GetIterable()) {
        for (unsigned i = 0; i < MemoryPool<Gameobject, GameobjectCreateParams>::objectsPerPage; i++) {
            Gameobject& obj = page[i].obj;
            if (!obj.Live()) continue;

            glm::vec3 relPos = (obj.Position() - currentCamera.position);
            glm::mat4x4 modelMatrix = glm::mat4x4(obj.GetRotSclMatrix());
            modelMatrix[3].x = relPos.x;
            modelMatrix[3].y = relPos.y;
            modelMatrix[3].z = relPos.z;
            modelMatrix[3].w = 1; // TODO is this one neccesary?
            obj.render.pool->StreamModelMatrix(obj.render.instanceIndex, modelMatrix);
            if (obj.normalMatDirty) {
                //obj.normalMatDirty = false; TODO: it's always dirty because we're streaming it;
                obj.render.pool->StreamNormalMatrix(obj.render.instanceIndex, glm::inverseTranspose(obj.GetRotSclMatrix())); // TODO: many things won't be nonuniformly scaled and this will be an unneccesary performance cost for them
            }
        }
    }

    // TODO: physobjects
}

GraphicsEngine::GraphicsEngine() {
    defaultDrawingPasses = {};  
    renderGraph = std::shared_ptr<RenderGraph>(new RenderGraph());  
}

GraphicsEngine::~GraphicsEngine() {

}

#include "graphics_engine.hpp"
#include "memory_pool.hpp"
#include "gameobject.hpp"
#include "static_meshpool.hpp"
#include <glm/gtc/matrix_inverse.hpp>
#include "rendergraph_node.hpp"

GraphicsEngine& GraphicsEngine::Get() {
    static GraphicsEngine GE;
    return GE;
}

void GraphicsEngine::RenderScene(double dt) {


    WriteModelMatrices();

    Meshpool::PrepareDraw();

    if (activeRenderGraph)
        activeRenderGraph->Render();

    glFlush();
    Meshpool::PrepareWrite();
}

void GraphicsEngine::UpdateRenderGraph(std::vector<std::shared_ptr<RenderPass>> nodes) {
    activeRenderGraph = std::make_shared<RenderGraph>(nodes);
}

void GraphicsEngine::WriteModelMatrices() {
    for (auto& page : MemoryPool<Gameobject>::Get().GetIterable()) {
        for (unsigned i = 0; i < MemoryPool<Gameobject>::objectsPerPage; i++) {
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
                obj.normalMatDirty = false;
                obj.render.pool->StreamNormalMatrix(obj.render.instanceIndex, glm::inverseTranspose(obj.GetRotSclMatrix())); // TODO: many things won't be nonuniformly scaled and this will be an unneccesary performance cost for them
            }
        }
    }

    // TODO: physobjects
}

GraphicsEngine::GraphicsEngine() {
    mainDrawingPass = std::make_shared<RenderPass>();
    mainDrawingPass->name = "default";
    mainDrawingPass->renderTarget = WindowRenderTargetDescriptor();
}

GraphicsEngine::~GraphicsEngine() {

}

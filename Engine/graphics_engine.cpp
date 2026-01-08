#include "graphics_engine.hpp"
#include "memory_pool.hpp"
#include "gameobject.hpp"
#include "static_meshpool.hpp"

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
        }
    }

    // TODO: physobjects
}

GraphicsEngine::GraphicsEngine() {

}

GraphicsEngine::~GraphicsEngine() {

}

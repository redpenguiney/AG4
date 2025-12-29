#include "graphics_engine.hpp"

GraphicsEngine& GraphicsEngine::Get() {
    static GraphicsEngine GE;
    return GE;
}

void GraphicsEngine::RenderScene(double dt) {

}

GraphicsEngine::GraphicsEngine() {

}

GraphicsEngine::~GraphicsEngine() {

}

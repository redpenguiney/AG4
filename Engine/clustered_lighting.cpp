#include "clustered_lighting.hpp"
#include "light.hpp"
#include "mainloop.hpp"
#include "graphics_engine.hpp"
#include "gameobject.hpp"

ClusteredLighting& ClusteredLighting::Get()
{
    static ClusteredLighting CL;
    return CL;
}

ClusteredLighting::ClusteredLighting() {

    GraphicsEngine::Get().AddUniformBuffer("lights", sizeof(Light) * 1024);

    preRenderConnection = Mainloop::Get().preRender.Connect([this](Mainloop*, float) {
        std::vector<Light> gpuLights;
        gpuLights.resize(lights.size() + 1);
        for (size_t i = 0; i < lights.size(); i++) {
            gpuLights[i].color = lights[i]->color;
            gpuLights[i].intensity = lights[i]->intensity;
            gpuLights[i].ambience = lights[i]->ambienceStrength;

            if (auto pl = std::dynamic_pointer_cast<PointLight>(lights[i])) {
                glm::dvec3 worldPos;
                if (pl->attachment) {
                    worldPos = pl->attachment->Position() + glm::dmat3x3(pl->attachment->GetRotSclMatrix()) * pl->position;
                }
                else {
                    worldPos = pl->position;
                }
                glm::vec3 relPos = worldPos - GraphicsEngine::Get().currentCamera.position;
                gpuLights[i].pos = relPos;
                gpuLights[i].lightType = 1.0f;
            }

            if (auto sl = std::dynamic_pointer_cast<SpotLight>(lights[i])) {
                gpuLights[i].lightType = 2.0f;
                gpuLights[i].spotlightInnerAngle = cosf(sl->innerAngle);
                gpuLights[i].spotlightOuterAngle = cosf(sl->outerAngle);
                if (sl->attachment) {
                    gpuLights[i].direction = -sl->attachment->ObjectNormalToWorld(sl->direction);
                }
                else {
                    gpuLights[i].direction = -sl->direction;
                }
            }
            else if (auto el = std::dynamic_pointer_cast<EnvironmentalLight>(lights[i])) {
                gpuLights[i].lightType = 3.0f;
                gpuLights[i].direction = el->direction;
            }
            else {
                Assert(std::dynamic_pointer_cast<PointLight>(lights[i]));
            }
        }
        gpuLights.back().lightType = 0.0f;

        GraphicsEngine::Get().UploadToUniformBuffer("lights", gpuLights.data(), sizeof(Light) * gpuLights.size(), 0);
        });
}

ClusteredLighting::~ClusteredLighting() {

}

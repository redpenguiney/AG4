#pragma once
#include <memory>
#include <vector>
#include <glm/vec3.hpp>

class Camera;
class Mesh;
class DrawPass;
class ConvexMeshPhysicsGeometry;
class Gameobject;

std::shared_ptr<Mesh> GetCubeMesh();
std::shared_ptr<Mesh> GetArrowMesh();

std::shared_ptr<DrawPass> GetDebugWireframePass();
std::shared_ptr<DrawPass> GetDebugSolidPass();
std::shared_ptr<ConvexMeshPhysicsGeometry> GetCubeCollisions();

std::vector<Gameobject*> BuildPit(glm::dvec3 pos, glm::vec3 size, float elasticity, float friction);
std::vector<Gameobject*> BuildCubeArray(glm::dvec3 origin, glm::dvec3 stride, glm::uvec3 nCubes, bool physics, float elasticity, float friction);

inline float freecamPitch = 0, freecamYaw = 0, freecamSpeed = 0;
std::shared_ptr<Camera> GetFreecam();

Gameobject* DebugArrow(glm::vec3 pos, glm::vec3 direction, glm::vec3 color);
Gameobject* DebugPoint(glm::dvec3 pos, glm::vec3 color = {1, 1, 1});
Gameobject* DebugLine(glm::dvec3 a, glm::dvec3 b, glm::vec3 color = { 1, 1, 1 });
Gameobject* DebugTriangle(glm::dvec3 a, glm::dvec3 b, glm::dvec3 c, glm::vec3 color = { 1, 1, 1 });
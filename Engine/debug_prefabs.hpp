#pragma once
#include <memory>
#include "glm/vec3.hpp"

class Mesh;
class DrawPass;
class ConvexMeshPhysicsGeometry;
class Gameobject;

std::shared_ptr<Mesh> GetCubeMesh();
std::shared_ptr<DrawPass> GetDebugWireframePass();
std::shared_ptr<DrawPass> GetDebugSolidPass();
std::shared_ptr<ConvexMeshPhysicsGeometry> GetCubeCollisions();

void BuildPit(glm::vec3 pos, glm::vec3 size, float elasticity, float friction);

Gameobject* DebugPoint(glm::dvec3 pos, glm::vec3 color = {1, 1, 1});
Gameobject* DebugLine(glm::dvec3 a, glm::dvec3 b, glm::vec3 color = { 1, 1, 1 });
Gameobject* DebugTriangle(glm::dvec3 a, glm::dvec3 b, glm::dvec3 c, glm::vec3 color = { 1, 1, 1 });
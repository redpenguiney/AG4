#pragma once
#include <memory>

class Mesh;
class DrawPass;
class ConvexMeshPhysicsGeometry;

std::shared_ptr<Mesh> GetCubeMesh();
std::shared_ptr<DrawPass> GetDebugWireframePass();
std::shared_ptr<ConvexMeshPhysicsGeometry> GetCubeCollisions();
#include "physics_mesh.hpp"
#include "mesh.hpp"
std::shared_ptr<ConvexMeshPhysicsGeometry> ConvexMeshPhysicsGeometry::FromMesh(const std::shared_ptr<Mesh>& m) {
    Assert(m->format.GetAttribute(SpecialVertexAttributeNames::VERTEX_POSITION) != nullptr);
    return std::shared_ptr<ConvexMeshPhysicsGeometry>(new ConvexMeshPhysicsGeometry());
}

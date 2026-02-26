#include "physics_mesh.hpp"
#include "mesh.hpp"

std::shared_ptr<ConvexMeshPhysicsGeometry> ConvexMeshPhysicsGeometry::FromMesh(const std::shared_ptr<Mesh>& m) {
    auto posAttribute = m->format.GetAttribute(SpecialVertexAttributeNames::VERTEX_POSITION);
    Assert(posAttribute != nullptr);

    std::vector<std::array<glm::vec3, 3>> triangles;
    triangles.resize(m->numIndices / 3);

    const auto& srcVerts = m->GetVertices();
    for (size_t triI = 0; triI < m->numIndices / 3; triI += 1) {
        for (size_t vI = 0; vI < 3; vI++) {
            for (size_t j = 0; j < 3; j++) {
                triangles[triI][vI][j] = srcVerts[(triI * 3 + vI) * m->format.ScalarsPerVertex() + posAttribute->offset / 4 + j].f;
            }
        }  
    }

    return std::shared_ptr<ConvexMeshPhysicsGeometry>(new ConvexMeshPhysicsGeometry(triangles));
}

ConvexMeshPhysicsGeometry::ConvexMeshPhysicsGeometry(std::vector<std::array<glm::vec3, 3>> tris): triangles(tris) {

}

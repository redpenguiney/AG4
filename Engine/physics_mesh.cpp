#include "physics_mesh.hpp"
#include "mesh.hpp"
#include <limits>
#include "gameobject.hpp"
#include <glm/gtx/projection.hpp>

RaycastResult ConvexMeshPhysicsGeometry::Raycast(glm::dvec3 rayOrigin, glm::dvec3 direction, Gameobject* object) {
    constexpr float epsilon = std::numeric_limits<float>::epsilon();

    // convert direction and position into object space
    // notice that we use floats here
    glm::mat3x3 toObjectSpace = glm::inverse(object->GetRotSclMatrix());
    glm::vec3 rayRelPos = glm::vec3(rayOrigin - object->Position());
    rayRelPos = toObjectSpace * rayRelPos;
    glm::vec3 rayRelDir = toObjectSpace * glm::vec3(direction);

    for (size_t i = 0; i < triangles.size(); i++) {
        // from https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
        glm::vec3 edge1 = triangles[i][1] - triangles[i][0];
        glm::vec3 edge2 = triangles[i][2] - triangles[i][0];
        glm::vec3 normal = glm::cross(edge1, edge2);

        if (glm::dot(rayRelDir, normal) > 0) continue;

        glm::vec3 rayCrossEdge2 = glm::cross(rayRelDir, edge2);
        float det = glm::dot(edge1, rayCrossEdge2);

        if (det > -epsilon && det < epsilon)
            continue;

        float inv_det = 1.0 / det;
        glm::vec3 s = rayRelPos - triangles[i][0];
        float u = inv_det * glm::dot(s, rayCrossEdge2);

        if ((u < 0 && abs(u) > epsilon) || (u > 1 && abs(u - 1) > epsilon))
            continue;

        glm::vec3 s_cross_e1 = cross(s, edge1);
        float v = inv_det * glm::dot(rayRelDir, s_cross_e1);

        if ((v < 0 && abs(v) > epsilon) || (u + v > 1 && abs(u + v - 1) > epsilon))
            continue;

        float t = inv_det * dot(edge2, s_cross_e1);

        if (t > epsilon) // ray intersection
        {
            glm::vec3 hitLocalPoint = rayRelPos + rayRelDir * t;
            glm::dvec3 hitPoint = glm::dvec3(object->GetRotSclMatrix() * hitLocalPoint) + object->Position();
            normal = glm::normalize(
                normal.x / object->Scale().x * object->GetRotSclMatrix()[0] +
                normal.y / object->Scale().y * object->GetRotSclMatrix()[1] +
                normal.z / object->Scale().z * object->GetRotSclMatrix()[2]
            );
            return RaycastResult{
                .hitPos = hitPoint,
                .hitNormal = normal,
                .distance = glm::length2(hitPoint - rayOrigin),
                .object = object
            };
        }
        else {
            continue;
        }
    }
    return RaycastResult{
        .object = nullptr
    };
}

std::shared_ptr<ConvexMeshPhysicsGeometry> ConvexMeshPhysicsGeometry::FromMesh(const std::shared_ptr<Mesh>& m) {
    auto posAttribute = m->format.GetAttribute(SpecialVertexAttributeNames::VERTEX_POSITION);
    Assert(posAttribute != nullptr);

    std::vector<std::array<glm::vec3, 3>> triangles;
    triangles.resize(m->numIndices / 3);

    const auto& srcVerts = m->GetVertices();
    const auto& srcIndices = m->GetIndices();
    for (size_t triI = 0; triI < m->numIndices / 3; triI += 1) {
        for (size_t vI = 0; vI < 3; vI++) {
            for (size_t j = 0; j < 3; j++) {
                unsigned index = srcIndices[triI * 3 + vI];
                triangles[triI][vI][j] = srcVerts[index * m->format.ScalarsPerVertex() + posAttribute->offset / 4 + j].f;
            }
        }  
    }

    return std::shared_ptr<ConvexMeshPhysicsGeometry>(new ConvexMeshPhysicsGeometry(triangles));
}

ConvexMeshPhysicsGeometry::ConvexMeshPhysicsGeometry(std::vector<std::array<glm::vec3, 3>> tris): triangles(tris) {

}

RaycastResult SpherePhysicsGeometry::Raycast(glm::dvec3 rayOrigin, glm::dvec3 rayDirection, Gameobject* object) {
    // convert direction and position into object space
    // notice that we use floats here
    glm::vec3 rayRelPos = glm::vec3(rayOrigin - object->Position());
    glm::vec3 rayRelDir = glm::vec3(rayDirection);
    float radiusSquared = object->Scale().x * object->Scale().x;
    if (glm::dot(rayRelPos, rayRelPos) < radiusSquared) { // then the ray started inside the sphere
        return RaycastResult{
            .hitPos = rayOrigin,
            .hitNormal = -rayDirection,
            .distance = 0,
            .object = object
        };
    }
    else {
        float d = glm::dot(rayRelPos, rayRelDir);
        if (d <= 0) {
            glm::vec3 v = rayRelPos - d;
            float vSquared = glm::dot(v, v);
            if (vSquared < radiusSquared) {
                glm::dvec3 hitPos = object->Position() + glm::dvec3(v - rayRelDir * glm::sqrt(radiusSquared - vSquared));
                return RaycastResult{
                    .hitPos = hitPos,
                    .hitNormal = glm::normalize(hitPos - object->Position()),
                    .distance = glm::length2(hitPos - rayOrigin),
                    .object = object
                };
            }
            else {
                return RaycastResult{
                    .object = nullptr
                };
            }
        }
        else {
            return RaycastResult{
                .object = nullptr
            };
        }
    }
    
    return RaycastResult();
}

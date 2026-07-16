#include "physics_mesh.hpp"
#include "mesh.hpp"
#include <limits>
#include "gameobject.hpp"
#include <glm/gtx/projection.hpp>
#include "let_me_hash_a_tuple.hpp"
#include <glm/gtx/hash.hpp>
#include <unordered_set>

RaycastResult ConvexMeshPhysicsGeometry::Raycast(glm::dvec3 rayOrigin, glm::dvec3 direction, Gameobject* object, RaycastParams params) {
    constexpr float epsilon = std::numeric_limits<float>::epsilon();

    // convert direction and position into object space
    // notice that we use floats here
    glm::mat3x3 toObjectSpace = glm::inverse(object->GetRotSclMatrix());
    glm::vec3 rayRelPos = glm::vec3(rayOrigin - object->Position());
    rayRelPos = toObjectSpace * rayRelPos;
    glm::vec3 rayRelDir = toObjectSpace * glm::vec3(direction);

    Assert(!params.preferMesh); // TODO

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

        float inv_det = 1.0f / det;
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
            
            return RaycastResult{
                .hitPos = hitPoint,
                .hitNormal = object->ObjectNormalToWorld(normal),
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
    // TODO: assert that mesh positions are normalized

    std::vector<std::array<glm::vec3, 3>> triangles;
    triangles.resize(m->numIndices / 3);

    const auto& srcVerts = m->GetVertices();
    const auto& srcIndices = m->GetIndices();

    std::vector<Polygon> polygons; 

    for (size_t triI = 0; triI < m->numIndices / 3; triI += 1) {
        for (size_t vI = 0; vI < 3; vI++) {
            for (size_t j = 0; j < 3; j++) {
                unsigned index = srcIndices[triI * 3 + vI];
                triangles[triI][vI][j] = srcVerts[index * m->format.ScalarsPerVertex() + posAttribute->offset / 4 + j].f;
            }
        }  

        glm::vec3 normal = glm::normalize(glm::cross(triangles[triI][2] - triangles[triI][0], triangles[triI][1] - triangles[triI][0]));
        if (glm::dot(normal, triangles[triI][0]) < 0) normal = -normal;

        // todo: poor time complexity
        for (auto& p : polygons) {
            if (glm::all(glm::epsilonEqual(p.normal, normal, 0.0001f))) {

                bool alreadyA = false, alreadyB = false, alreadyC = false;
                for (auto& point : p.points) {
                    if (point == triangles[triI][0]) alreadyA = true;
                    if (point == triangles[triI][1]) alreadyB = true;
                    if (point == triangles[triI][2]) alreadyC = true;
                }
                if (!alreadyA) p.points.push_back(triangles[triI][0]);
                if (!alreadyB) p.points.push_back(triangles[triI][1]);
                if (!alreadyC) p.points.push_back(triangles[triI][2]);

                goto normalAlreadyExists;
            }
        }
        polygons.push_back(Polygon{ 
            .points = {triangles[triI][0], triangles[triI][1], triangles[triI][2]},
            .normal = normal
            }); 
        normalAlreadyExists:;
    }

    std::array<std::vector<glm::vec3>, 8> supportVerts;
    for (size_t vertI = 0; vertI < srcVerts.size() / m->format.ScalarsPerVertex(); vertI++) {
        glm::vec3 vert;
        vert.x = srcVerts[vertI * m->format.ScalarsPerVertex() + posAttribute->offset / 4].f;
        vert.y = srcVerts[vertI * m->format.ScalarsPerVertex() + posAttribute->offset / 4 + 1].f;
        vert.z = srcVerts[vertI * m->format.ScalarsPerVertex() + posAttribute->offset / 4 + 2].f;
        
        size_t index = 0;
        if (vert.x >= 0) index += 4;
        if (vert.y >= 0) index += 2;
        if (vert.z >= 0) index += 1;

        supportVerts[index].push_back(vert);
    }

    std::unordered_set<glm::vec3> uniqueEdgeDirections;
    for (auto& p : polygons) {
        // sort polygon points into clockwise order about normal
        glm::vec3 sum(0, 0, 0);
        for (glm::vec3& point : p.points) sum += point;
        glm::vec3 centre = sum / static_cast<float>(p.points.size());
        
        std::sort(p.points.begin(), p.points.end(), [&](const glm::vec3& v1, const glm::vec3& v2) {
            return glm::dot(p.normal, glm::cross(v1 - centre, v2 - centre)) > 0;
            });

        // extract edges
        for (unsigned i = 0; i < p.points.size(); i++) {
            glm::vec3 edge = glm::normalize(p.points[i] - p.points[i + 1 == p.points.size() ? 0 : i + 1]);
            uniqueEdgeDirections.insert(edge);
        }
    }
    std::vector<glm::vec3> edges(uniqueEdgeDirections.begin(), uniqueEdgeDirections.end());
    return std::shared_ptr<ConvexMeshPhysicsGeometry>(new ConvexMeshPhysicsGeometry(m, triangles, supportVerts, polygons, edges));
}



glm::vec3 ConvexMeshPhysicsGeometry::Support(glm::vec3 direction) const {
    size_t index = 0;
    if (direction.x >= 0) index += 4;
    if (direction.y >= 0) index += 2;
    if (direction.z >= 0) index += 1;

    // Weird meshes could potentially have this array be empty, in which case we have to iterate through all 8 arrays.
    if (!supportVertices[index].empty()) {
        glm::vec3 currentBest = supportVertices[index][0];
        float bestDot = glm::dot(direction, currentBest);
        for (size_t i = 1; i < supportVertices[index].size(); i++) {
            float dot = glm::dot(direction, supportVertices[index][i]);
            if (dot > bestDot) {
                bestDot = dot;
                currentBest = supportVertices[index][i];
            }
        }
        return currentBest;
    }
    else {
        float bestDot = -INFINITY;
        glm::vec3 currentBest;
        for (auto& arr : supportVertices) {
            for (auto& v : arr) {
                float dot = glm::dot(direction, v);
                if (dot > bestDot) {
                    bestDot = dot;
                    currentBest = v;
                }
            }
        }
        return currentBest;
    }
}

static float TetrahedronVolume(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
    return glm::dot(a, glm::cross(b, c)) / 6.0f;
}

// helper function for CalculateLocalMomentOfInertia(). i is matrix x, j is matrix y.
static float ComputeInertiaProduct(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, unsigned int i, unsigned int j) {
    return (
        2.0f * p1[i] * p1[j] + p2[i] * p3[j] + p3[i] * p2[j] +
        2.0f * p2[i] * p2[j] + p1[i] * p3[j] + p3[i] * p1[j] +
        2.0f * p3[i] * p3[j] + p1[i] * p2[j] + p2[i] * p1[j]
        );
}

// helper function for CalculateLocalMomentOfInertia(). i is the matrix x-coordinate.
static float ComputeInertiaMoment(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, unsigned int i) {
    return (
        pow(p1[i], 2.0f) + p2[i] * p3[i] +
        pow(p2[i], 2.0f) + p1[i] * p3[i] +
        pow(p3[i], 2.0f) + p1[i] * p2[i]
        );
}

float ConvexMeshPhysicsGeometry::Volume(glm::vec3 objectScale) {
    float volume = 0;
    for (const auto& triangle : triangles) {
        glm::vec3 p1 = triangle[0] * objectScale;
        glm::vec3 p2 = triangle[1] * objectScale;
        glm::vec3 p3 = triangle[2] * objectScale;

        glm::vec3 triangleNormal = glm::cross(p2 - p1, p3 - p1);

        glm::vec3 triangleCentroid = (p1 + p2 + p3) / 3.0f;

        float tetrahedronVolume = TetrahedronVolume(p1, p2, p3, { 1.0, 1.0, 1.0 });

        float dot = glm::dot(triangleNormal, triangleCentroid); // We're checking if the triangle normal points towards the origin.
        if (dot > 0.0) { // then the triangle's normal points away from the origin. 
            volume += tetrahedronVolume;
        }
        else { // then the triangle normal points towards the origin (because mesh has concave bits) and we gotta negate all the values it calculates.
            volume -= tetrahedronVolume;
        }
    }
    return volume;
}

void ConvexMeshPhysicsGeometry::AddLocalMomentOfInertiaContribution(glm::vec3& centerOfMass, float& Ia, float& Ib, float& Ic, float& Iap, float& Ibp, float& Icp, glm::vec3 objectScale, float density) {
    for (const auto& triangle : triangles) {
        glm::vec3 p1 = triangle[0] * objectScale;
        glm::vec3 p2 = triangle[1] * objectScale;
        glm::vec3 p3 = triangle[2] * objectScale;

        glm::vec3 triangleNormal = glm::cross(p2 - p1, p3 - p1);

        glm::vec3 triangleCentroid = (p1 + p2 + p3) / 3.0f;
        glm::vec3 tetrahedronCenterOfMass = (p1 + p2 + p3) / 4.0f; // not bothering to add the origin for obvious reasons

        // volume calc from https://math.stackexchange.com/questions/3616760/how-to-calculate-the-volume-of-tetrahedron-given-by-4-points
        float tetrahedronVolume = TetrahedronVolume(p1, p2, p3, { 1.0, 1.0, 1.0 });

        float tetrahedronMass = tetrahedronVolume * density;

        float dot = glm::dot(triangleNormal, triangleCentroid); // We're checking if the triangle normal points towards the origin.
        if (dot < 0.0) { // then the triangle's normal points towards the origin and must be negated. 
            tetrahedronMass *= -1;
            tetrahedronVolume *= -1;
        }

        centerOfMass += tetrahedronCenterOfMass * tetrahedronMass; // we'll divide it to get actual average at the end

        // from 23:00 in the video i mentioned above
        Ia += 6.0f * tetrahedronVolume * (ComputeInertiaMoment(p1, p2, p3, 1) + ComputeInertiaMoment(p1, p2, p3, 2));
        Ib += 6.0f * tetrahedronVolume * (ComputeInertiaMoment(p1, p2, p3, 0) + ComputeInertiaMoment(p1, p2, p3, 2));
        Ic += 6.0f * tetrahedronVolume * (ComputeInertiaMoment(p1, p2, p3, 0) + ComputeInertiaMoment(p1, p2, p3, 1));
        Iap += 6.0f * tetrahedronVolume * ComputeInertiaProduct(p1, p2, p3, 1, 2);
        Ibp += 6.0f * tetrahedronVolume * ComputeInertiaProduct(p1, p2, p3, 0, 1);
        Icp += 6.0f * tetrahedronVolume * ComputeInertiaProduct(p1, p2, p3, 0, 2);
    }
}

ConvexMeshPhysicsGeometry::ConvexMeshPhysicsGeometry(
    const std::shared_ptr<Mesh>& src,
    std::vector<std::array<glm::vec3, 3>> tris, 
    std::array<std::vector<glm::vec3>, 8> support, 
    std::vector<Polygon> polygons, 
    std::vector<glm::vec3> edges)
    : 
    source(src), triangles(tris), supportVertices(support), polygons(polygons), uniqueEdgeDirections(edges) 
{

}

RaycastResult SpherePhysicsGeometry::Raycast(glm::dvec3 rayOrigin, glm::dvec3 rayDirection, Gameobject* object, RaycastParams params) {
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

glm::vec3 SpherePhysicsGeometry::Support(glm::vec3 direction) const {
    return direction * 0.5f; // radius is 0.5
}

void SpherePhysicsGeometry::AddLocalMomentOfInertiaContribution(glm::vec3& centerOfMass, float& Ia, float& Ib, float& Ic, float& Iap, float& Ibp, float& Icp, glm::vec3 objectScale, float density) {
    float mass = Volume(objectScale) * density;
    float radiusSquared = 0.25f * objectScale.x * objectScale.x;
    Ia += mass * radiusSquared * 0.4f;
    Ib += mass * radiusSquared * 0.4f;
    Ic += mass * radiusSquared * 0.4f;
}

float SpherePhysicsGeometry::Volume(glm::vec3 objectScale) {
    return 4.0f / 3.0f * 3.1415926f * objectScale.x * objectScale.x * objectScale.x;
}

glm::mat3x3 BasePhysicsGeometry::GetMomentOfInertia(glm::vec3 objectScale, float objectMass) {
    Assert(objectMass > 0);
    Assert(objectScale.x > 0 && objectScale.y > 0 && objectScale.z > 0);

    // From http://number-none.com/blow/inertia/body_i.html and https://stackoverflow.com/questions/809832/how-can-i-compute-the-mass-and-moment-of-inertia-of-a-polyhedron 
    // and most especially https://www.youtube.com/watch?v=GYc99lMdcFE

    float volume = Volume(objectScale);
    Assert(volume > 0);

    float density = objectMass / volume;

    glm::vec3 objectCenterOfMass = { 0, 0, 0 };
    float Ia = 0.0, Ib = 0.0, Ic = 0.0, Iap = 0.0, Ibp = 0.0, Icp = 0.0; // components of inertia tensor. i think.

    AddLocalMomentOfInertiaContribution(objectCenterOfMass, Ia, Ib, Ic, Iap, Ibp, Icp, objectScale, objectMass);

    objectCenterOfMass /= objectMass;
    Ia *= density / 60.0f;
    Ib *= density / 60.0f;
    Ic *= density / 60.0f;
    Iap *= density / 120.0f;
    Iap *= density / 120.0f;
    Iap *= density / 120.0f;

    // We just calculated inertia tensor with respect to the origin. Since all meshes are transformed to be centered on the origin, we're done here.
    glm::mat3x3 inertiaTensor{
        Ia, -Ibp, -Icp,
        -Ibp, Ib, -Iap,
        -Icp, -Iap, Ic
    };

    return inertiaTensor;
}

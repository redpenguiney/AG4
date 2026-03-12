#include "collision_detection.hpp"
#include <memory>
#include "collider.hpp"
#include "gameobject.hpp"
#include <vector>
#include <array>
#include "glm/vec3.hpp"
#include "glm/mat3x3.hpp"
#include "glm/mat4x4.hpp"

#ifndef GLM_CONFIG_XYZW_ONLY 
#error bruh
#endif

static float SignedDistanceToPlane(glm::vec3 planeNormal, glm::vec3 point, glm::vec3 pointOnPlane) {
    return glm::dot(planeNormal, point - pointOnPlane);
}

static void ValidateVector(glm::vec3 vec) {
    Assert(!std::isnan(vec.x) && !std::isnan(vec.y) && !std::isnan(vec.z));
}

static Polygon ClipFaces() {

}

// Helper function to get face normals of the polytope in a's object space.
    // Returns vector of pair {normal, distance to face} and index of the closest normal.
static std::pair<std::vector<std::pair<glm::vec3, float>>, size_t> GetFaceNormals(const std::vector<unsigned int>& faces, const std::vector<std::array<glm::vec3, 3>>& polytope) {
    std::vector<std::pair<glm::vec3, float>> tnormals;
    Assert(faces.size() > 0);
    size_t minTriangle = 0;
    float  minDistance = FLT_MAX;

    // std::cout << "There are " << faces.size() << " face indices.\n";
    for (size_t i = 0; i < faces.size(); i += 3) {
        auto& a = polytope[faces[i]];
        auto& b = polytope[faces[i + 1]];
        auto& c = polytope[faces[i + 2]];

        glm::vec3 normal = glm::normalize(glm::cross(b[0] - a[0], c[0] - a[0]));
        double distance = glm::dot(normal, a[0]);

        if (distance < 0) {
            normal *= -1;
            distance *= -1;
        }

        // std::cout << "Pushing back to normals.\n";
        tnormals.emplace_back(std::make_pair(normal, std::isnan(distance) ? INFINITY : distance));

        if (distance < minDistance) {
            minTriangle = i / 3;
            minDistance = distance;
            // std::cout << "Min distance " << minDistance << " created by dot of " << glm::to_string(normal) << " and support " << glm::to_string(a[0]) << "\n";
        }
    }

    Assert(tnormals.size() > 0);
    return { tnormals, minTriangle };
};

static std::optional<Collision> CollideGJKEPA(Gameobject* a, Gameobject* b) {    
    auto pmA = std::dynamic_pointer_cast<ConvexPhysicsGeometry>(a->GetCollider()->physicsMesh);
    auto pmB = std::dynamic_pointer_cast<ConvexPhysicsGeometry>(b->GetCollider()->physicsMesh);
    Assert(pmA && pmB);
    
    glm::mat3x3 worldToB = glm::inverse(b->GetRotSclMatrix());
    glm::vec3 bToARelPos = a->Position() - b->Position();
    glm::mat3x3 worldToA = glm::inverse(a->GetRotSclMatrix());
    //glm::mat3x4 bToA = bToWorld;
    //bToWorld[3] = bToARelPos;
    

    // simplex and search direction are in object space of A
    // first vec3 in each array is actual simplex point, other two are the points on the collider surface in respective object space (needed to find contact points later)
    std::vector < std::array<glm::vec3, 3>> simplex;
    glm::vec3 searchDirection;

    // HELPER FUNCTION DEFINITIONS

    auto NewSimplexPoint = [&](glm::vec3 direction) -> std::array<glm::vec3, 3> {
        ValidateVector(direction);
        glm::vec3 supportA = pmA->Support(direction);
        glm::vec3 directionInBSpace = worldToB * a->GetRotSclMatrix() * direction;

        glm::vec3 supportB = pmB->Support(-directionInBSpace);
        glm::vec3 bInASpace = worldToA * ((b->GetRotSclMatrix() * supportB) + bToARelPos);
        return { supportA - bInASpace, supportA, supportB };
        };

    auto LineCase = [&]() -> bool {
        auto& a = simplex[0];
        auto& b = simplex[1];

        auto ab = b[0] - a[0];
        auto ao = -a[0]; // a to origin

        // https://www.youtube.com/watch?app=desktop&v=MDusDn8oTSE 5:43 has a nice picture to illustrate this
        // in this case, the 2 points of the simplex describe 2 parallel planes whose volume contain the origin if the vector between the 2 points is within 90 degrees of the vector from one of the points to to the origin
        if (glm::dot(ab, ao) >= 0) {
            glm::vec3 cross = glm::cross(ab, ao);
            if (glm::length2(cross) > 0) {
                // make search direction go towards origin again
                searchDirection = glm::normalize(glm::cross(cross, ab));
                return false;
            }
            else { // ab and ao are the same direction (only occurs when collider vertices are all perfectly aligned except on normal axis). 
                // we'll use the normal of the plane defined by a[1], a[2] in objectA space, and b[1] for our search direction instead.
                //glm::vec3 aa = a[0] - b[1];
                //searchDirection = glm::normalize(glm::cross(aa, ab));
                //ValidateVector(searchDirection);
                //return false;
                Assert(glm::length2(ab) >= glm::length2(ao));
                return true;
            }
        }
        else { // if the condition failed, the 1st point is between 2nd point and the origin and thus the 2nd point won't help determine whether simplex contains the origin
            simplex = { a };
            searchDirection = ao;
            return false;
        }
        };

    auto TriangleCase = [&]() {
        auto& a = simplex[0];
        auto& b = simplex[1];
        auto& c = simplex[2];

        auto ab = b[0] - a[0];
        auto ac = c[0] - a[0];
        auto ao = -a[0]; // (a to origin)

        auto abc = glm::normalize(glm::cross(ab, ac)); // normal of the plane defined by the 3 points of the simplex

         if (glm::dot(glm::cross(abc, ac), ao) >= 0) {
            if (glm::dot(ac, ao) >= 0) {
                simplex = { a, c };
                searchDirection = glm::normalize(glm::cross(glm::cross(ac, ao), ac));
            }
            else {
                simplex = { a, b };
                LineCase();
            }
        }
        else {
            if (glm::dot(glm::cross(ab, abc), ao) >= 0) {
                simplex = { a, b };
                LineCase();
            }
            else {
                if (glm::dot(abc, ao) >= 0) {
                    searchDirection = abc;
                }
                else {
                    simplex = { a, c, b };
                    searchDirection = -abc;
                }
            }
        }
        searchDirection = glm::normalize(searchDirection);
        };

    auto TetrahedronCase = [&]() -> bool {
        auto& a = simplex[0];
        auto& b = simplex[1];
        auto& c = simplex[2];
        auto& d = simplex[3];

        auto ab = b[0] - a[0];
        auto ac = c[0] - a[0];
        auto ad = d[0] - a[0];
        auto ao = -a[0];

        // These are the normals of the 3 triangles in the tetrahedron. (the 4th triangle normal, bcd, is not needed because the triangle case checked it)
        auto abc = glm::cross(ab, ac);
        auto acd = glm::cross(ac, ad);
        auto adb = glm::cross(ad, ab);

        // if it's on the inside of all 3 of these triangles, then collision detected.
        // if it's in front of a triangle's normal, remove the simplex point not included in that triangle, and search in front of that normal for a point.
        if (glm::dot(abc, ao) >= 0) {
            simplex = { a, b, c };
            TriangleCase();
            return false;
        }
        else if (glm::dot(acd, ao) >= 0) {
            simplex = { a, c, d };
            TriangleCase();
            return false;
        }
        else if (glm::dot(adb, ao) >= 0) {
            simplex = { a, d, b };
            TriangleCase();
            return false;
        }

        return true;
        };

    auto ContactFromLineCase = [&]() -> std::optional<Collision> {
        Assert(simplex.size() == 2);
        glm::vec3 normal = glm::normalize(simplex[1][0] - simplex[0][0]);
        ValidateVector(normal);

        return Collision{
            .collisionPoints = {{normal * -0.5f, normal * 0.5f},},
            .collisionNormal = normal,
        };
        };

    auto EPA = [&]() -> std::optional<Collision> {
        // If simplex doesn't already have 4 vertices (possible with some edge cases), we need to add some.
            // see https://allenchou.net/2013/12/game-physics-contact-generation-epa/
        if (simplex.size() == 1) {
            Assert(false);
        }
        if (simplex.size() == 2) {
            constexpr std::array<glm::vec3, 3> axes = {
                glm::vec3{1, 0, 0}, {0, 1, 0}, {0, 0, 1}
            };
            glm::vec3 lineDir = simplex[1][0] - simplex[0][0];

            glm::vec3 searchDir;
            if (std::abs(lineDir.x) < std::abs(lineDir.y) && std::abs(lineDir.x) < std::abs(lineDir.z)) {
                searchDir = glm::cross(lineDir, axes[0]);
            }
            else if (std::abs(lineDir.y) < std::abs(lineDir.z)) {
                searchDir = glm::cross(lineDir, axes[1]);
            }
            else {
                searchDir = glm::cross(lineDir, axes[2]);
            }

            glm::quat rot = glm::angleAxis(glm::pi<float>(), lineDir);

            std::array<glm::vec3, 3> simplexPoint;
            for (unsigned i = 0; i < 6; i++) {
                simplexPoint = NewSimplexPoint(searchDir);
                auto cross = glm::cross(simplexPoint[0] - simplex[0][0], lineDir);
                if (glm::length2(cross) > 0.0001f) break;
            }
            simplex.insert(simplex.begin(), simplexPoint);
        }
        if (simplex.size() == 3) {
            glm::vec3 triNormal = glm::cross(simplex[1][0] - simplex[0][0], simplex[2][0] - simplex[0][0]);

            std::array<glm::vec3, 3> simplexPoint = NewSimplexPoint(triNormal);
            if (std::abs(SignedDistanceToPlane(triNormal, simplexPoint[0], simplex[0][0])) < 0.0001f) {
                simplexPoint = NewSimplexPoint(-triNormal);
            }
            simplex.insert(simplex.begin(), simplexPoint);
        }

        // Simplex is no longer a simplex and is just a convex polytope (3d polygon) made from (more than 4) points on the Minkoski difference.
        auto& polytope = simplex;

        // To find the normal, we must progressively expand the simplex, which neccesitates knowing the faces of the simplex so that we can calculate proper normals
        std::vector<unsigned int> faces = {0, 1, 2,   0, 3, 1,   0, 2, 3,   1, 3, 2};

        
        auto [normals, minFace] = GetFaceNormals(faces, polytope);
        Assert(normals.size() == 4); // initial simplex should have 4 vertices and 4 faces

        glm::vec3 minNormal;
        float minDistance = FLT_MAX;
        unsigned nIterations = 0;
        while (minDistance == FLT_MAX) {
            nIterations += 1;

            minNormal = normals.at(minFace).first;
            minDistance = normals.at(minFace).second;

            if (nIterations > 64) {
                DebugLogError("EPA failed (iterations exceeded)");
                break;
            }

            auto support = NewSimplexPoint(minNormal);
            double sDistance = glm::dot(minNormal, support[0]);

            if (abs(sDistance - minDistance) > 0.00001) {
                std::vector<std::pair<unsigned int, unsigned int>> uniqueEdges;

                auto AddIfUniqueEdge = [&](unsigned int e1, unsigned int e2) {
                    auto reverse = std::find(
                        uniqueEdges.begin(),
                        uniqueEdges.end(),
                        std::make_pair(faces[e1], faces[e2])
                    );
                    if (reverse != uniqueEdges.end()) {
                        uniqueEdges.erase(reverse);
                        return;
                    }

                    uniqueEdges.emplace_back(faces[e1], faces[e2]);
                    };

                Assert(normals.size() > 0);
                Assert(normals.size() * 3 == faces.size());
                for (unsigned int i = 0; i < normals.size(); i++) {
                    if (SignedDistanceToPlane(normals[i].first, polytope[faces[i * 3]][0], support[0]) < 0) {
                        unsigned int f = i * 3;

                        // For all of the edges of this face, 
                        AddIfUniqueEdge(f, f + 1);
                        AddIfUniqueEdge(f + 1, f + 2);
                        AddIfUniqueEdge(f + 2, f);

                        faces[f + 2] = faces.back(); faces.pop_back();
                        faces[f + 1] = faces.back(); faces.pop_back();
                        faces[f] = faces.back(); faces.pop_back();

                        normals[i] = normals.back(); // pop-erase
                        normals.pop_back();

                        i--;
                    }

                }
                if (uniqueEdges.size() == 0) {
                    DebugLogError("EPA failed (edges)");
                    break;
                }
                std::vector<unsigned int> newFaces;
                for (auto [edgeIndex1, edgeIndex2] : uniqueEdges) {
                    newFaces.push_back(edgeIndex1);
                    newFaces.push_back(edgeIndex2);
                    newFaces.push_back(polytope.size());
                }

                polytope.push_back(support);

                Assert(newFaces.size() > 0);
                auto [newNormals, newMinFace] = GetFaceNormals(newFaces, polytope);

                double oldMinDistance = FLT_MAX;
                for (unsigned int i = 0; i < normals.size(); i++) {
                    if (normals[i].second < oldMinDistance) {
                        oldMinDistance = normals[i].second;
                        minFace = i;
                    }
                }

                if (newNormals.at(newMinFace).second < oldMinDistance) {
                    minFace = newMinFace + normals.size();
                }

                Assert(newFaces.size() == 3 * newNormals.size());
                faces.insert(faces.end(), newFaces.begin(), newFaces.end());
                normals.insert(normals.end(), newNormals.begin(), newNormals.end());
                Assert(normals.size() * 3 == faces.size());
                minDistance = FLT_MAX;
            }
        }

        Assert(minNormal != glm::vec3(0, 0, 0));

        if (faces.size() == 0) {
            DebugLogError("EPA failed (faces)");
            Assert(false);
        }

        auto convexMeshA = std::dynamic_pointer_cast<ConvexMeshPhysicsGeometry>(a->GetCollider()->physicsMesh);
        auto convexMeshB = std::dynamic_pointer_cast<ConvexMeshPhysicsGeometry>(b->GetCollider()->physicsMesh);
        if (convexMeshA && convexMeshB) { // then we can just clip the faces to extract full contact information
            return ClipFaces()
        }
        else { // use barycentric coords. only get one contact point this way but that'll have to be enough
            // find collision point
            // get verts of closest triangle to origin
            auto& pA = polytope[faces[minFace * 3]];
            auto& pB = polytope[faces[minFace * 3 + 1]];
            auto& pC = polytope[faces[minFace * 3 + 2]];

            // Project origin onto plane abc to get point of contact in minkoski space
            auto planeToOrigin = -pA[0];
            auto distance = glm::dot(planeToOrigin, minNormal);
            auto projectedPoint = -minNormal * distance;

            // put that point of contact in barycentric coordinates (meaning its a mix of the triangle vertices), so that we can get out of minkoski space and into world space
            auto ab = pB[0] - pA[0];
            auto ac = pC[0] - pA[0];
            auto ao = projectedPoint - pA[0];

            float d00 = glm::dot(ab, ab);
            float d01 = glm::dot(ab, ac);
            float d11 = glm::dot(ac, ac);
            float d20 = glm::dot(ao, ab);
            float d21 = glm::dot(ao, ac);
            float denom = d00 * d11 - d01 * d01;

            // uvw is barycentric coords aka a mixture of the triangle vertices that averages out to the point we got
            float v = (d11 * d20 - d01 * d21) / denom;
            float w = (d00 * d21 - d01 * d20) / denom;
            float u = 1.0 - v - w;
            // we use that mixture with the triangle vertices that AREN'T in minkoski space to get the real contact point
            // std::cout << "We got " << glm::to_string((a[0] * u) + (b[0] * v) + (c[0] * w)) << " vs " << glm::to_string(projectedPoint);
            glm::vec3 pointForObj1 = (pA[1] * u) + (pB[1] * v) + (pC[1] * w);
            glm::vec3 pointForObj2 = (pA[2] * u) + (pB[2] * v) + (pC[2] * w);

            return Collision{
                .collisionPoints = {{-pointForObj1, -pointForObj2},}, // TODO: why does negating them fix it? it shouldn't 
                .collisionNormal = a->ObjectNormalToWorld(minNormal),
            };
        }


        
        };

    // initial simplex/search direction
    simplex.push_back(NewSimplexPoint(glm::normalize(a->Position() - b->Position()))); // arbitrary intial search direction
    searchDirection = glm::normalize(-simplex.back()[0]);


    // GJK LOOP

    unsigned nIterations = 0;
    while (true) {
        nIterations++;
        if (nIterations == 64) {
            DebugLogError("WARNING: GJK FAILED TO DETERMINE COLLISION AFTER 64 ITERATIONS. NANs likely.");
            return std::nullopt;
        }

        // get new point for simplex
        auto newSimplexPoint = NewSimplexPoint(searchDirection);

        // this is the farthest point in this direction, so if it didn't get past the origin, then origin is gonna be outside the minoski difference meaning no collision.
        // TODO: maybe try other technique (see if distance has decreased)
        if (glm::dot(newSimplexPoint[0], searchDirection) <= 0) {
            return std::nullopt;
        }

        // add point to simplex
        // we gotta insert at beginning because simplex order matters
        simplex.insert(simplex.begin(), newSimplexPoint);

        // 1. see if origin intersects simplex
        // 2. if it does and we have 4 points, collision detected!
        // 3. if it does but not 4 points yet, compute new search direction and go back to beginning of loop to find more points
        // 4. if it doesn't, we have an unneccesary point in the simplex, reduce the simplex to closest/most relevant stuff to origin by doing some dot/cross product stuff, compute new search direction, and go back to beginning of loop to find more points
        switch (simplex.size()) {
        case 2:
            if (LineCase()) {
                auto collisioninfo = EPA();
                return collisioninfo;
            }
            ValidateVector(searchDirection);
            break;
        case 3:
            TriangleCase();
            ValidateVector(searchDirection);
            break;
        case 4:
            if (TetrahedronCase()) { // this function is not void like the others, returns true if collision confirmed
                auto collisioninfo = EPA();
                return collisioninfo;
            }
            ValidateVector(searchDirection);
            break;
        default:
            DebugLogError("GJK: WHAT");
            Assert(false);
            break;
        }
    }
}

static std::optional<Collision> CollideSAT(Gameobject* a, Gameobject* b) {
    auto pmA = std::dynamic_pointer_cast<ConvexMeshPhysicsGeometry>(a->GetCollider()->physicsMesh);
    auto pmB = std::dynamic_pointer_cast<ConvexMeshPhysicsGeometry>(b->GetCollider()->physicsMesh);
    Assert(pmA && pmB);

    glm::mat3x3 worldToB = glm::inverse(b->GetRotSclMatrix());
    glm::vec3 bToARelPos = a->Position() - b->Position();
    glm::mat3x3 worldToA = glm::inverse(a->GetRotSclMatrix());

    float greatestSeperation = -INFINITY;
    // in A space
    glm::vec3 collisionNormal;
      
    for (auto& planeA : pmA->planes) {
        glm::vec3 normalInBspace = worldToB * a->GetRotSclMatrix() * planeA.first;
        glm::vec3 pointB = pmB->Support(-normalInBspace);
        
        float distance = SignedDistanceToPlane(planeA.first, pointB, planeA.second);
        if (distance >= 0) {
            return std::nullopt;
        }
        else if (distance > greatestSeperation) {
            greatestSeperation = distance;
            collisionNormal = planeA.first;
        }
    }

    for (auto& planeB : pmB->planes) {
        glm::vec3 normalInAspace = worldToA * b->GetRotSclMatrix() * planeB.first;
        glm::vec3 pointA = pmB->Support(-normalInAspace);

        float distance = SignedDistanceToPlane(planeB.first, pointA, planeB.second);
        if (distance >= 0) {
            return std::nullopt;
        }
        else if (distance > greatestSeperation) {
            greatestSeperation = distance;
            collisionNormal = normalInAspace;
        }
    }

    // handle edge pairs
    /*for (auto& edgeA : pmA->edges) {
        for (auto& edgeB : pmB->edges) {

        }
    }*/

}

std::optional<Collision> NarrowphaseCollisionDetection(Gameobject* a, Gameobject* b) {
    auto sphereA = std::dynamic_pointer_cast<SpherePhysicsGeometry>(a->GetCollider()->physicsMesh);
    auto sphereB = std::dynamic_pointer_cast<SpherePhysicsGeometry>(b->GetCollider()->physicsMesh);
    auto convexA = std::dynamic_pointer_cast<ConvexPhysicsGeometry>(a->GetCollider()->physicsMesh);
    auto convexB = std::dynamic_pointer_cast<ConvexPhysicsGeometry>(b->GetCollider()->physicsMesh);
    auto convexMeshA = std::dynamic_pointer_cast<ConvexMeshPhysicsGeometry>(a->GetCollider()->physicsMesh);
    auto convexMeshB = std::dynamic_pointer_cast<ConvexMeshPhysicsGeometry>(b->GetCollider()->physicsMesh);

    //if (convexMeshA && convexMeshB) {
        //return CollideSAT(a, b);
    //} else
    if (convexA && convexB) {
        return CollideGJKEPA(a , b);
    }
    else {
        DebugLogError("Incompatible/unimplemented narrowphase collision detection type pair. No collision reported.");
        return std::nullopt;
    }
}

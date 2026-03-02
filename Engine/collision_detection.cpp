#include "collision_detection.hpp"
#include <memory>
#include "collider.hpp"
#include "gameobject.hpp"
#include <vector>
#include <array>
#include "glm/vec3.hpp"
#include "glm/mat3x3.hpp"
#include "glm/mat4x4.hpp"

static std::optional<Collision> CollideGJKEPA(Gameobject* a, Gameobject* b) {
    auto pmA = std::dynamic_pointer_cast<ConvexPhysicsGeometry>(a->GetCollider()->physicsMesh);
    auto pmB = std::dynamic_pointer_cast<ConvexPhysicsGeometry>(b->GetCollider()->physicsMesh);
    Assert(pmA && pmB);
    
    // simplex and search direction are in object space of A
    // first vec3 in each array is actual simplex point, other two are the points on the collider surface in object space (needed to find contact points later)
    std::vector < std::array<glm::vec3, 3>> simplex;

    searchDirection = glm::normalize(-simplex.back()[0]);

    auto NewSimplexPoint = [&]() {

        };

    unsigned nIterations = 0;
    while (true) {
        nIterations++;
        if (nIterations == 64) {
            DebugLogError("WARNING: GJK FAILED TO DETERMINE COLLISION AFTER 64 ITERATIONS. NANs likely.");
            return std::nullopt;
        }

        // get new point for simplex
        auto newSimplexPoint = NewSimplexPoint(searchDirection, collider1, collider2, invNormMatrix1, invNormMatrix2, transform1.GetPhysicsModelMatrix(), transform2.GetPhysicsModelMatrix());

        // this is the farthest point in this direction, so if it didn't get past the origin, then origin is gonna be outside the minoski difference meaning no collision.
        if (glm::dot(newSimplexPoint[0], searchDirection) <= 0) {
            std::unique_lock l3(collisionCacheMutex);
            collisionCache[pair] = std::nullopt;
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
            LineCase(simplex, searchDirection);
            break;
        case 3:
            TriangleCase(simplex, searchDirection);
            break;
        case 4:
            if (TetrahedronCase(simplex, searchDirection)) { // this function is not void like the others, returns true if collision confirmed

                auto collisioninfo = EPA(simplex, collider1, collider2, invNormMatrix1, invNormMatrix2, transform1.GetPhysicsModelMatrix(), transform2.GetPhysicsModelMatrix(), invPhysMatrix2, transform1.GetNormalMatrix());
                std::unique_lock l4(collisionCacheMutex);
                collisionCache[pair] = collisioninfo;
                if (&transform1 > &transform2) {
                    collisionCache[pair]->collisionNormal *= -1;
                    std::swap(collisionCache[pair]->contactPoints, collisionCache[pair]->otherContactPoints);
                }
                return collisioninfo;
                // std::cout << "THERE IS A COLLISION\n";
                // std::cout << "Positions are #1 = " << glm::to_string(transform1.Position()) << " and #2 = " << glm::to_string(transform2.Position()) << "\n";
                //auto result = FindContact(transform1, collider1, transform2, collider2);
                //if (!result) {
                    //std::cout << "SAT and GJK disagreed, uh oh.\n";
                //}
                //return result;
            }
            break;
        default:
            DebugLogError("GJK: WHAT");
            Assert(false);
            break;
        }
    }
}

std::optional<Collision> NarrowphaseCollisionDetection(Gameobject* a, Gameobject* b) {
    auto sphereA = std::dynamic_pointer_cast<SpherePhysicsGeometry>(a->GetCollider()->physicsMesh);
    auto sphereB = std::dynamic_pointer_cast<SpherePhysicsGeometry>(b->GetCollider()->physicsMesh);
    auto convexA = std::dynamic_pointer_cast<ConvexPhysicsGeometry>(a->GetCollider()->physicsMesh);
    auto convexB = std::dynamic_pointer_cast<ConvexPhysicsGeometry>(b->GetCollider()->physicsMesh);

    if (convexA && convexB) {
        return CollideGJKEPA(a , b);
    }
    else {
        DebugLogError("Incompatible/unimplemented narrowphase collision detection type pair. No collision reported.");
        return std::nullopt;
    }
}

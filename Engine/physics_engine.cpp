#include "physics_engine.hpp"
#include "gameobject.hpp"
#include "collision_detection.hpp"
#include <unordered_set>
#include "glm/vec3.hpp"
#include "glm/ext/quaternion_float.hpp"
#include "let_me_hash_a_tuple.hpp"

PhysicsEngine& PhysicsEngine::Get() {
	static PhysicsEngine PE;
	return PE;
}

// Jacobi-solve version of https://matthias-research.github.io/pages/publications/PBDBodies.pdf
void PhysicsEngine::StepSimulation(double timestep) {
	auto simulate = [&](auto iterable) mutable {

		// we do this one dynamically. warmstarting is useless for infinitely stiff constraints
		staticCollisions.clear();
		dynamicCollisions.clear();
		std::unordered_set<std::pair<Gameobject*, Gameobject*>, hash_pair::hash<Gameobject*, Gameobject*>> alreadyCheckedCollisions;

		
		for (auto& page : iterable) {
			for (unsigned i = 0; i < POOL_OBJECTS_PER_PAGE; i++) {
				Physobject& obj = page[i].obj;
				if (!obj.Live()) continue;
				obj.lastPos = obj.position;
				obj.lastRot = obj.rotation;

				obj.SetPosition(obj.lastPos + glm::dvec3(obj.velocity) * timestep + gravity * 0.5 * timestep * timestep);
				obj.SetRotation(obj.lastRot + 0.5f * glm::quat(0, obj.rotVelocity));

				
			}
		}

		// Collect collisions
		for (auto& page : iterable) {
			for (unsigned i = 0; i < POOL_OBJECTS_PER_PAGE; i++) {
				Physobject& obj = page[i].obj;
				if (!obj.Live()) continue;

				if (obj.collider) {
					// 
					auto potentiallyColliding = GameobjectSAS().QueryAABB(obj.collider->aabb);
					for (auto other : potentiallyColliding) {
						if (alreadyCheckedCollisions.contains({ &obj, other->object }) || alreadyCheckedCollisions.contains({ other->object, &obj })) {
							continue;
						}
						else {
							auto result = NarrowphaseCollisionDetection(&obj, other->object);
							if (result) {
								alreadyCheckedCollisions.insert({ &obj, other->object });
								for (auto& contactPoint : result->collisionPoints) {
									if (auto otherPhys = dynamic_cast<Physobject*>(other->object)) {
										dynamicCollisions.push_back(DynamicCollisionConstraint{
											.r1 = contactPoint.first,
											.r2 = contactPoint.second,
											.a = &obj,
											.b = otherPhys,
											.collisionNormal = result->collisionNormal,
											});
									}
									else {
										staticCollisions.push_back(StaticCollisionConstraint{
											.r1 = contactPoint.first,
											.r2 = contactPoint.second,
											.a = &obj,
											.b = other->object,
											.collisionNormal = result->collisionNormal,
											});
									}
								}
							}
						}
					}
				}
			}
		}

		// todo: REDUNDANT AABBTREE UPDATES
		for (unsigned posIter = 0; posIter < 1; posIter++) {
			for (auto& collision : staticCollisions) {
				// todo: could maybe evaluate these in A's object space and then use floats?
				glm::dvec3 r1 = glm::dvec3(collision.a->GetRotSclMatrix() * collision.r1) + collision.a->Position();
				glm::dvec3 r2 = glm::dvec3(collision.b->GetRotSclMatrix() * collision.r2) + collision.b->Position();
				glm::dvec3 dnormal = glm::dvec3(collision.collisionNormal);
				double penetration = glm::dot(r1 - r2, dnormal);
				if (penetration < 0) continue;

				glm::vec3 torqueAxis1 = glm::cross(collision.r1, collision.collisionNormal);
				// TODO: untested
				float reducedInverseMass1 = collision.a->inverseMass + glm::dot(torqueAxis1, collision.a->inverseInertiaTensor * torqueAxis1);
			
				float lagrange = - penetration / reducedInverseMass1;
				glm::vec3 impulse = collision.collisionNormal * lagrange;
				glm::dvec3 displacement = impulse * reducedInverseMass1;
				collision.a->SetPosition(collision.a->Position() + displacement);
				collision.a->SetRotation(glm::normalize(collision.a->Rotation() + 0.5f * glm::quat(0, collision.a->inverseInertiaTensor * glm::cross(collision.r1, impulse))));
			}
		}

		for (auto& page : iterable) {
			for (unsigned i = 0; i < POOL_OBJECTS_PER_PAGE; i++) {
				Physobject& obj = page[i].obj;
				if (!obj.Live()) continue;
				
				obj.velocity = (obj.Position() - obj.lastPos) / timestep;
				obj.rotVelocity = 2.0f * glm::vec3(obj.rotVelocity.x, obj.rotVelocity.y, obj.rotVelocity.z) / (float)timestep;
				//obj.SetPosition(obj.position);
				//obj.SetRotation(obj.rotation);
			}
		}
		};
	simulate(Physobject::Pool::Get().GetIterable()); // note: not extendable to subclasses this way. do not copy paste
}

PhysicsEngine::PhysicsEngine() {

}

PhysicsEngine::~PhysicsEngine() {

}

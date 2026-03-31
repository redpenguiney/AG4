#include "physics_engine.hpp"
#include "gameobject.hpp"
#include "collision_detection.hpp"
#include <unordered_set>
#include "glm/vec3.hpp"
#include "glm/ext/quaternion_float.hpp"
#include "let_me_hash_a_tuple.hpp"
#include "debug_prefabs.hpp"

PhysicsEngine& PhysicsEngine::Get() {
	static PhysicsEngine PE;
	return PE;
}

// see https://matthias-research.github.io/pages/publications/PBDBodies.pdf
// TODO:
/*
- angular velocity gradually declines, probably due to numerical instability
*/
void PhysicsEngine::StepSimulation(double timestep) {
	auto simulate = [&](auto iterable) mutable {

		// we do collision constraints dynamically. warmstarting is useless for infinitely stiff constraints
		staticCollisions.clear();
		dynamicCollisions.clear();
		std::unordered_set<std::pair<Gameobject*, Gameobject*>, hash_pair::hash<Gameobject*, Gameobject*>> alreadyCheckedCollisions;

		// Integrate velocities
		for (auto& page : iterable) {
			for (unsigned i = 0; i < POOL_OBJECTS_PER_PAGE; i++) {
				Physobject& obj = page[i].obj;
				if (!obj.Live()) continue;
				
				//DebugLogInfo("Rot vel ", obj.rotVelocity);

				obj.lastPos = obj.position;
				obj.lastRot = obj.rotation;
				obj.SetPosition(obj.position + glm::dvec3(obj.velocity) * timestep + gravity * 0.5 * timestep * timestep);
				obj.SetRotation(glm::normalize(obj.rotation + 0.5f * (float)timestep * glm::quat(0, obj.rotVelocity.x, obj.rotVelocity.y, obj.rotVelocity.z) * obj.rotation));
				
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
						if (other == obj.collider.get()) continue;
						if (alreadyCheckedCollisions.contains({ &obj, other->object }) || alreadyCheckedCollisions.contains({ other->object, &obj })) {
							continue;
						}
						else {
							auto result = NarrowphaseCollisionDetection(&obj, other->object);
							if (result) {
								alreadyCheckedCollisions.insert({ &obj, other->object });
								// use centroid of contact points
									// TODO: this is inaccurate centroid calculation; we should decompose it into triangles, then take weighted average of centroids of those triangles
									// (or just handle each contact point seperately)
								glm::vec3 r1 = {0, 0, 0}, r2 = {0, 0, 0};
								for (auto& contactPoint : result->collisionPoints) {
									r1 += contactPoint.first;
									r2 += contactPoint.second;
									/*if (auto otherPhys = dynamic_cast<Physobject*>(other->object)) {
										dynamicCollisions.push_back(DynamicCollisionConstraint{
											.r1 = contactPoint.first,
											.r2 = contactPoint.second,
											.a = &obj,
											.b = otherPhys,
											.collisionNormal = result->collisionNormal,
											.nerf = 1.0f / (float)result->collisionPoints.size()
											});
									}
									else {
										staticCollisions.push_back(StaticCollisionConstraint{
											.r1 = contactPoint.first,
											.r2 = contactPoint.second,
											.a = &obj,
											.b = other->object,
											.collisionNormal = result->collisionNormal,
											.nerf = 1.0f / (float)result->collisionPoints.size()
											});
									}*/
								}
								r1 /= static_cast<float>(result->collisionPoints.size());
								r2 /= static_cast<float>(result->collisionPoints.size());
								if (auto otherPhys = dynamic_cast<Physobject*>(other->object)) {
									dynamicCollisions.push_back(DynamicCollisionConstraint{
										.r1 = r1,
										.r2 = r2,
										.a = &obj,
										.b = otherPhys,
										.collisionNormal = result->collisionNormal,
										.totalLagrange = 0
										});
								}
								else {
									staticCollisions.push_back(StaticCollisionConstraint{
										.r1 = r1,
										.r2 = r2,
										.a = &obj,
										.b = other->object,
										.collisionNormal = result->collisionNormal,
										.totalLagrange = 0
										});
								}
							}
						}
					}
				}
			}
		}

		// todo: see paper Nonconvex Rigid Bodies with Stacking. Since we aren't doing a Jacobi solve, we could handle contacts in a more stable order 

		//if (!staticCollisions.empty()) DebugLogInfo("SOLVING");

		// todo: REDUNDANT AABBTREE UPDATES
		// Run physics solver 
		for (unsigned posIter = 0; posIter < 1; posIter++) {

			for (auto& collision : staticCollisions) {
				glm::vec3 r1 = collision.a->GetRotSclMatrix() * collision.r1;// +collision.a->Position();
				glm::vec3 r2 = collision.b->GetRotSclMatrix() * collision.r2;// +collision.b->Position();
				
				//DebugPoint(glm::dvec3(r1) + collision.a->Position());
				//DebugPoint(glm::dvec3(r2) + collision.b->Position(), {0.5, 0.5, 0.5});

				if (posIter == 0) {
					collision.relV = collision.a->velocity + r1 * collision.a->rotVelocity;
				}

				// todo: could maybe evaluate these in A's object space and then use floats?
				glm::dvec3 dnormal = glm::dvec3(collision.collisionNormal);
				double penetration = glm::dot(glm::dvec3(r2) + collision.b->Position() - glm::dvec3(r1) - collision.a->Position(), dnormal);
				if (penetration < 0) continue;

				glm::vec3 torqueAxis1 = glm::cross(r1, collision.collisionNormal);
				// TODO: untested

				DebugLogInfo("N ", dnormal, " R1 ", r1);

				float inertiaAroundTorqueAxis = 0;
				if (glm::length2(torqueAxis1) != 0) {
					auto localAxis = glm::inverse(collision.a->Rotation()) * glm::normalize(torqueAxis1);
					inertiaAroundTorqueAxis = glm::dot(localAxis, collision.a->inverseInertiaTensor * localAxis);
				}
				//DebugLogInfo("MMOI ", inertiaAroundTorqueAxis, " or ", glm::length2(torqueAxis1) * inertiaAroundTorqueAxis);
				float reducedInverseMass1 = collision.a->inverseMass + glm::length2(torqueAxis1) * inertiaAroundTorqueAxis;
			
				float lagrange = penetration / reducedInverseMass1;
				collision.totalLagrange += lagrange;
				glm::vec3 impulse = collision.collisionNormal * lagrange;
				glm::dvec3 displacement = impulse * collision.a->inverseMass;
				//DebugLogInfo("Displacement strength ", glm::length(displacement), " against penetration ", penetration);
				glm::vec3 torque = glm::cross(r1, impulse);
				//glm::vec3 dRot = inertiaAroundTorqueAxis * torque;
				glm::vec3 dRot = collision.a->inverseInertiaTensor * torque;
 				collision.a->SetPosition(collision.a->Position() + displacement);
				collision.a->SetRotation(glm::normalize(collision.a->Rotation() + 0.5f * glm::quat(0, dRot.x, dRot.y, dRot.z) * collision.a->Rotation()));

				//DebugLogInfo("TORQUE ", torque)

				//glm::dvec3 newR1 = glm::dvec3(collision.a->GetRotSclMatrix() * collision.r1) + collision.a->Position();
				//double newPenetration = glm::dot(r2 - newR1, dnormal);
				//DebugLogInfo("DIS ", displacement);
				//collision.a->SetPosition(collision.a->Position() + dnormal * penetration);
			}
		}

		// Derive new velocities
		for (auto& page : iterable) {
			for (unsigned i = 0; i < POOL_OBJECTS_PER_PAGE; i++) {
				Physobject& obj = page[i].obj;
				if (!obj.Live()) continue;
				
				obj.velocity = (obj.Position() - obj.lastPos) / timestep;
				glm::quat newRot = obj.Rotation();
				//obj.rotVelocity = 2.0f * glm::vec3(
				//	obj.lastRot.w * newRot.x - obj.lastRot.x * newRot.w - obj.lastRot.y * newRot.z + obj.lastRot.z * newRot.y,
				//	obj.lastRot.w * newRot.y + obj.lastRot.x * newRot.z - obj.lastRot.y * newRot.w - obj.lastRot.z * newRot.x,
				//	obj.lastRot.w * newRot.z - obj.lastRot.x * newRot.y + obj.lastRot.y * newRot.x - obj.lastRot.z * newRot.w
				//) / (float)timestep;
				glm::quat dRot = newRot * glm::inverse(obj.lastRot);
				obj.rotVelocity = 2.0f * glm::vec3(dRot.x, dRot.y, dRot.z) / (float)timestep;
				if (dRot.w < 0) {
					obj.rotVelocity *= -1;
					//DebugLogInfo("Fipped rot");
				}
				// prevents numerical precision errors causing rotvelocity to accumulate
				//if (glm::length2(obj.rotVelocity) < 0.0000001) obj.rotVelocity = glm::vec3(0,0,0);
				//obj.SetPosition(obj.position);
				//obj.SetRotation(obj.rotation);
			}
		}

		// Apply friction/restitution/etc.
		for (auto& collision : staticCollisions) {
			glm::vec3 r1 = glm::dvec3(collision.a->GetRotSclMatrix() * collision.r1);
			glm::vec3 currentRelV = collision.a->velocity + r1 * collision.a->rotVelocity;
			float currentNormalSpeed = glm::dot(collision.collisionNormal, currentRelV);
			float priorNormalSpeed = glm::dot(collision.collisionNormal, collision.relV);
			//if (priorNormalSpeed > -0.001) priorNormalSpeed = 0.0f; // prevent jitter and backwards restitution
			glm::vec3 tangentVelocity = collision.relV - collision.collisionNormal * priorNormalSpeed;
			float tangentSpeed = glm::length(tangentVelocity);

			float restitution = 1;// collision.a->elasticity * 0.5f; // TODO 0.5f should be replaced with property of B
			float normalForce = collision.totalLagrange / (float)timestep / (float)timestep;
			float friction = 0;// collision.a->friction * 0.5f; // TODO 0.5f should be replaced with property of B
			glm::vec3 deltaV = collision.collisionNormal * (-currentNormalSpeed + std::min(0.0f, -restitution * priorNormalSpeed));
			if (tangentSpeed != 0) {
				glm::vec3 frictionDirection = -tangentVelocity / tangentSpeed;
				deltaV += frictionDirection * glm::min(normalForce * friction, tangentSpeed);
			}

			glm::vec3 torqueAxis1 = glm::cross(r1, collision.collisionNormal);
			float inertiaAroundTorqueAxis = 0;
			if (glm::length2(torqueAxis1) != 0) {
				auto localAxis = glm::inverse(collision.a->Rotation()) * glm::normalize(torqueAxis1);
				inertiaAroundTorqueAxis = glm::dot(localAxis, collision.a->inverseInertiaTensor * localAxis);
			}
			glm::vec3 impulse = deltaV / (collision.a->inverseMass + glm::length2(torqueAxis1) * inertiaAroundTorqueAxis);
			collision.a->velocity += impulse * collision.a->inverseMass;
			collision.a->rotVelocity += impulse * inertiaAroundTorqueAxis * torqueAxis1;

		}
		};
	simulate(Physobject::Pool::Get().GetIterable()); // note: not extendable to subclasses this way. do not copy paste
}

PhysicsEngine::PhysicsEngine() {

}

PhysicsEngine::~PhysicsEngine() {

}

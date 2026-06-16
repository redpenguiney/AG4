#include "physics_engine.hpp"
#include "gameobject.hpp"
#include "collision_detection.hpp"
#include <unordered_set>
#include "glm/vec3.hpp"
#include "glm/ext/quaternion_float.hpp"
#include "let_me_hash_a_tuple.hpp"
#include "debug_prefabs.hpp"
#include <algorithm>

PhysicsEngine& PhysicsEngine::Get() {
	static PhysicsEngine PE;
	return PE;
}

// see https://matthias-research.github.io/pages/publications/PBDBodies.pdf (Jacobi solve variant of it)
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
				
				obj.lastPos = obj.position;
				obj.lastRot = obj.rotation;
				obj.SetPosition(obj.position + glm::dvec3(obj.velocity) * timestep + gravity * 0.5 * timestep * timestep);
				obj.SetRotation(glm::normalize(obj.rotation + 0.5f * (float)timestep * glm::quat(0, obj.rotVelocity.x, obj.rotVelocity.y, obj.rotVelocity.z) * obj.rotation));
				obj.nextPos = obj.position;
				obj.nextRot = obj.rotation;
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
								//glm::vec3 r1 = {0, 0, 0}, r2 = {0, 0, 0};
								// shuffle contactPoints because resolving them in a different order every frame improves stability (irrelevant with Jaocbi solve(
								//std::ranges::shuffle(result->collisionPoints.begin(), result->collisionPoints.end(), rng);
								for (auto& contactPoint : result->collisionPoints) {
									//r1 += contactPoint.first;
									//r2 += contactPoint.second;
									if (auto otherPhys = dynamic_cast<Physobject*>(other->object)) {
										dynamicCollisions.push_back(DynamicCollisionConstraint{
											.r1 = contactPoint.first,
											.r2 = contactPoint.second,
											.a = &obj,
											.b = otherPhys,
											.collisionNormal = result->collisionNormal,
											.totalNormalLagrange = 0,
											.nerf = 1.0f / static_cast<float>(result->collisionPoints.size())
											});
									}
									else {
										staticCollisions.push_back(StaticCollisionConstraint{
											.r1 = contactPoint.first,
											.r2 = contactPoint.second,
											.a = &obj,
											.b = other->object,
											.collisionNormal = result->collisionNormal,
											.totalNormalLagrange = 0,
											.nerf = 1.0f / static_cast<float>(result->collisionPoints.size())
											});
									}
								}
								/*r1 /= static_cast<float>(result->collisionPoints.size());
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
								}*/
							}
						}
					}
				}
			}
		}

		// todo: see paper Nonconvex Rigid Bodies with Stacking. We could handle contacts in a more stable order if we abandoned the Jacobi solve and solved individual collisions in parallel 
		 
		//if (staticCollisions.size() > 0)
			//DebugLogInfo(staticCollisions[0].r1);

		//if (!staticCollisions.empty()) DebugLogInfo("SOLVING (h=", timestep, ")");



		// Run physics solver 
		unsigned N_POS_ITERS = 1;
		for (unsigned posIter = 0; posIter < N_POS_ITERS; posIter++) {
			
			for (auto& collision : staticCollisions) {
				collision.PositionPass((float)timestep, posIter);
			}
			for (auto& collision : dynamicCollisions) {
				collision.PositionPass((float)timestep, posIter);
			}

			if (posIter != N_POS_ITERS - 1)
			for (auto& page : iterable) {
				for (unsigned i = 0; i < POOL_OBJECTS_PER_PAGE; i++) {
					Physobject& obj = page[i].obj;
					if (!obj.Live()) continue;

					obj.SetPosition(obj.nextPos);
					obj.SetRotation(obj.nextRot);
				}
			}
		}

		// Derive new velocities
		for (auto& page : iterable) {
			for (unsigned i = 0; i < POOL_OBJECTS_PER_PAGE; i++) {
				Physobject& obj = page[i].obj;
				if (!obj.Live()) continue;

				//DebugLogInfo("RV " , obj.rotVelocity);

				obj.SetPosition(obj.nextPos);
				obj.SetRotation(glm::normalize(obj.nextRot));

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

				obj.nextVel = obj.velocity;
				obj.nextRotVel = obj.rotVelocity;
			}
		}


		for (unsigned i = 0; i < 2; i++) {

			// Apply friction/restitution/etc.
			for (auto& collision : staticCollisions) {
				collision.VelocityPass((float)timestep);
				collision.nerf = 1.0f;
			}
			for (auto& collision : dynamicCollisions) {
				collision.VelocityPass((float)timestep);
				collision.nerf = 1.0f;
			}

			// TODO: could merge this with first pass of next frame
			for (auto& page : iterable) {
				for (unsigned i = 0; i < POOL_OBJECTS_PER_PAGE; i++) {
					Physobject& obj = page[i].obj;
					if (!obj.Live()) continue;
					obj.velocity = obj.nextVel;
					obj.rotVelocity = obj.nextRotVel;
				}
			}

		}

		};
	simulate(Physobject::Pool::Get().GetIterable()); // note: not extendable to subclasses this way. do not copy paste
}

PhysicsEngine::PhysicsEngine(): rng() {
}

PhysicsEngine::~PhysicsEngine() {

}

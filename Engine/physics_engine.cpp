#include "physics_engine.hpp"
#include "gameobject.hpp"
#include "collision_detection.hpp"

PhysicsEngine& PhysicsEngine::Get() {
	static PhysicsEngine PE;
	return PE;
}

void PhysicsEngine::StepSimulation(double timestep) {
	auto simulate = [&](auto iterable) mutable {

		std::unordered_map<std::pair<Gameobject*, Gameobject*>, CollisionConstraint*> collisions;

		for (auto& page : iterable) {
			for (unsigned i = 0; i < POOL_OBJECTS_PER_PAGE; i++) {
				Physobject& obj = page[i].obj;
				if (!obj.Live()) continue;
				obj.lastPos = obj.position;
				obj.lastRot = obj.rotation;

				obj.nextPos = obj.lastPos + glm::dvec3(obj.velocity) * timestep + gravity * 0.5 * timestep * timestep;
				obj.nextRot = obj.lastRot + 0.5f * glm::quat(0, obj.rotVelocity);

				if (obj.collider) {
					auto potentiallyColliding = GameobjectSAS().QueryAABB(obj.collider->aabb);
					for (auto other : potentiallyColliding) {
						if (collisions.contains({ &obj, other->object }) || collisions.contains({ other->object, &obj })) {
							continue;
						}	
						else {
							auto result = NarrowphaseCollisionDetection(&obj, other->object);
							if (result) {
								DebugLogInfo("YES COLLISION: ", result->collisionNormal);
							}
						}
					}
				}
			}
		}

		for (auto& page : iterable) {
			for (unsigned i = 0; i < POOL_OBJECTS_PER_PAGE; i++) {
				Physobject& obj = page[i].obj;
				if (!obj.Live()) continue;
				
				obj.velocity = (obj.nextPos - obj.lastPos) / timestep;
				obj.rotVelocity = 2.0f * glm::vec3(obj.rotVelocity.x, obj.rotVelocity.y, obj.rotVelocity.z) / (float)timestep;
				obj.SetPosition(obj.nextPos);
				obj.SetRotation(obj.nextRot);
			}
		}
		};
	simulate(Physobject::Pool::Get().GetIterable()); // note: not extendable to subclasses this way. do not copy paste
}

PhysicsEngine::PhysicsEngine() {

}

PhysicsEngine::~PhysicsEngine() {

}

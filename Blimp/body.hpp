#pragma once
#include "ik.hpp"
#include <event.hpp>

class Gameobject;
class Physobject;
class Body;
class Camera;

class BodyController {
public:
	Body* body = nullptr;
	
	virtual ~BodyController() = default;

	// called before drawing
	virtual void Update(float dt) = 0;

	// called before physics
	virtual void FixedUpdate(float dt) = 0;

};

class LocalPlayerController: public BodyController {
public:
	LocalPlayerController();

	std::shared_ptr<Camera> camera;

	void Update(float dt) override;
	void FixedUpdate(float dt) override;
	
};

struct BodyCreateParams {
	float height = 1.8f;
	float radius = 0.3f;
};

class Body {
public:
	Body(std::unique_ptr<BodyController> controller, BodyCreateParams params);

	void SetPos(glm::dvec3);

private:
	friend class LocalPlayerController;

	std::vector<Connection> conns;
	std::unique_ptr<BodyController> controller;
	// the collider doing all the physics
	Physobject* collider;
	std::vector<std::unique_ptr<Gameobject>> gameobjects;
	IKSkeletonInfo ik;
};
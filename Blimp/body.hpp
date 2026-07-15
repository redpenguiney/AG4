#pragma once
#include "ik.hpp"
#include <event.hpp>

class Gameobject;
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

class Body {
public:
	Body(std::unique_ptr<BodyController> controller);

private:
	friend class LocalPlayerController;

	Connection updateConn;
	Connection fixedUpdateConn;
	std::unique_ptr<BodyController> controller;
	std::vector<std::unique_ptr<Gameobject>> gameobjects;
	IKSkeletonInfo ik;
};
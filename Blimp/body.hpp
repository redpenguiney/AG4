#pragma once
#include "ik.hpp"
#include <event.hpp>
#include <transform_sync.hpp>
#include <optional>

class Gameobject;
class Physobject;
class Body;
class Camera;
class InputContext; 

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

	// x is forwards, y is rightwards
	glm::ivec2 moveInput = glm::ivec2(0, 0);

	float lookYaw;
	float lookPitch;

	std::shared_ptr<Camera> camera;

	std::unique_ptr<InputContext> inputContext;
	Connection inputConn;

	void Update(float dt) override;
	void FixedUpdate(float dt) override;
	
};

class RemotePlayerController: public BodyController {
public:
	RemotePlayerController(Player* owner);
	Player* const owner;

	void Update(float dt) override;
	void FixedUpdate(float dt) override;
};

struct BodyCreateParams {
	float height = 1.8f;
	float radius = 0.3f;
	std::optional<TransformSyncId> syncId = std::nullopt;
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
	std::unique_ptr<Physobject> collider;
	IKSkeletonInfo ik;
};
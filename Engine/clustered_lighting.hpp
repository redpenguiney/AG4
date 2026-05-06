#pragma once
#include <memory>
#include <string>
#include <vector>
#include <glm/vec3.hpp>
#include "event.hpp"

class RenderPass;

class Gameobject;

struct BaseLight {
	glm::vec3 color = { 1, 1, 1 };
	float intensity = 6.0f;
	float ambienceStrength = 0.2f;

	virtual ~BaseLight() = default;
};

struct PointLight : public BaseLight {
	glm::dvec3 position;

	// if provided, position will be in object space of the attachment.
	// todo: what if not std::shared_ptr?
	std::shared_ptr<Gameobject> attachment = nullptr;

	~PointLight() = default;
};

struct SpotLight : public PointLight {
	// must be normalized
	glm::vec3 direction;
	float innerAngle;
	float outerAngle;

	~SpotLight() = default;
};

struct EnvironmentalLight : public BaseLight {
	// must be normalized
	glm::vec3 direction;
	~EnvironmentalLight() = default;
};

class ClusteredLighting {
public:
	// Add this renderpass to your render graph and make renderpasses that use lights have it as a dependency.
	//static std::shared_ptr<RenderPass> GetLightingRenderpass();
	
	static ClusteredLighting& Get();

	const static inline std::string lightDependencyName = "lights";

	// change as you please
	// TODO: different datastructure perhaps?
	std::vector<std::shared_ptr<BaseLight>> lights;

	Connection preRenderConnection;
private:
	ClusteredLighting();
	~ClusteredLighting();
};
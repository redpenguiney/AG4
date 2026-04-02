#pragma once
#include <glm/vec3.hpp>

// Lighting struct for sending to the GPU. 
struct Light {
	// (using floating origin)
	glm::vec3 pos;
	float intensity;
	glm::vec3 color;
	float angle;
	glm::vec3 direction;
	// 1.0f = pointlight, 2.0f = spotlight, 3.0f = environmental light. 0.0f if STOP
	float lightType;
};
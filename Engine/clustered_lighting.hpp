#pragma once
#include <memory>
#include <string>

class RenderPass;
class BaseLight {
public:
	// Add this renderpass to your render graph and make renderpasses that use lights have it as a dependency.
	static std::shared_ptr<RenderPass> GetLightingRenderpass();



protected:
	
};

class PointLight : public BaseLight {
public:
	constexpr static inline std::string PointLightBufferName = "pointLights";

};
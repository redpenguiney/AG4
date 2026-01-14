#include "gameobject.hpp"
#include "mainloop.hpp"
#include "mesh_provider.hpp"
#include "mesh.hpp"
#include "graphics_engine.hpp"
#include "window.hpp"
#include "utility.hpp"
#include <algorithm>

int main() {
	Window::Get();
	GraphicsEngine::Get();

	DebugLogInfo("Main function reached successfully.");
	auto mparams = MeshCreateParams::Default();
	mparams.LoadObj("../models/rainbowcube.obj");

	auto squareMesh = Mesh::New(std::move(mparams));

	GameobjectCreateParams p;
	p.mesh = squareMesh;

	std::vector<std::shared_ptr<Gameobject>> objects;
	for (unsigned i = 0; i < 1; i++) {
		Gameobject* gameObj = Gameobject::New(p);
		//delete gameObj;
		std::shared_ptr<Gameobject> unique(gameObj);
		objects.push_back(unique);
	}
	
	// Freecam
	float pitch = 0, yaw = 0;
	Mainloop::Get().preRender->Connect([&pitch, &yaw](float) {
		pitch += Window::Get().MOUSE_DELTA.y;
		yaw += Window::Get().MOUSE_DELTA.x;
		pitch = std::clamp(pitch, -3.141f / 180 * -89, 3.141f / 180 * 89);
		if (yaw < 0) yaw += 3.141 * 2;
		yaw = std::fmod(yaw, 3.141f * 2);

		auto& cam = GraphicsEngine::Get().currentCamera;
		double forward = double(Window::Get().PRESSED_KEYS.contains(InputObject::W)) - double(Window::Get().PRESSED_KEYS.contains(InputObject::S));
		double right = double(Window::Get().PRESSED_KEYS.contains(InputObject::D)) - double(Window::Get().PRESSED_KEYS.contains(InputObject::A));
		double up = double(Window::Get().PRESSED_KEYS.contains(InputObject::Q)) - double(Window::Get().PRESSED_KEYS.contains(InputObject::E));

		cam.position += LookVector(pitch, yaw) * glm::dvec3(right, up, forward);
		cam.rotation = glm::quatLookAt(LookVector(pitch, yaw), glm::dvec3(0, 1, 0));
		});

	Mainloop::Get().Run();
	// TODO cleanup?

	DebugLogInfo("Main function body executed successfully.");
}
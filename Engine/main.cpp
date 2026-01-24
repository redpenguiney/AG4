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
	for (int i = 0; i < 5; i++) {
		Gameobject* gameObj = Gameobject::New(p);
		gameObj->SetPosition({ i, 0, -5 - i });
		gameObj->SetScale({ 0.8, 0.8, 0.8 });
		glm::vec4 color(1, 0, 1, 1);
		gameObj->SetInstanceAttribute(*squareMesh->format.GetAttribute("color"), color);
		gameObj->SetInstanceAttribute(*squareMesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);

		//delete gameObj;
		std::shared_ptr<Gameobject> unique(gameObj);
		objects.push_back(unique);

	}
	
	// Freecam
	float pitch = 0, yaw = 0, speed = 0;
	Mainloop::Get().preRender->Connect([&pitch, &yaw, &speed](float) {
		pitch += 0.01 * Window::Get().MOUSE_DELTA.y;
		yaw += 0.01 * Window::Get().MOUSE_DELTA.x;
		pitch = std::clamp(pitch, -glm::radians(89.0f), glm::radians(89.0f));
		if (yaw < 0) yaw += glm::radians(360.0f);
		yaw = std::fmod(yaw,glm::radians(360.0f));

		auto& cam = GraphicsEngine::Get().currentCamera;
		float forward = (Window::Get().PRESSED_KEYS.contains(InputObject::W) ? 1 : 0) - (Window::Get().PRESSED_KEYS.contains(InputObject::S) ? 1 : 0);
		float right = (Window::Get().PRESSED_KEYS.contains(InputObject::D) ? 1 : 0) - (Window::Get().PRESSED_KEYS.contains(InputObject::A) ? 1 : 0);
		float up = (Window::Get().PRESSED_KEYS.contains(InputObject::Q) ? 1 : 0) - (Window::Get().PRESSED_KEYS.contains(InputObject::E) ? 1 : 0);

		if (forward == 0 && right == 0 && up == 0) speed = 0;
		speed += 0.1;
		cam.rotation = glm::rotate(glm::rotate(glm::identity<glm::mat4x4>(), pitch, glm::vec3(1, 0, 0)), yaw, glm::vec3(0, 1, 0));
		glm::vec3 fDir = LookVector(pitch, yaw);
		glm::vec3 rDir= LookVector(0, yaw + glm::radians(90.0f));
		glm::vec3 upDir = glm::cross(fDir, rDir);
		cam.position += (fDir * forward + rDir * right + upDir * up) * speed;
		});

	Mainloop::Get().Run();
	// TODO cleanup?

	DebugLogInfo("Main function body executed successfully.");
}
#include "game_state.hpp"
#include "debug_prefabs.hpp"
#include <window.hpp>
#include <mainloop.hpp>

void GameState::MakeGameplay() {
	for (auto& obj : BuildPit({ 0, -10, 0 }, { 1000, 10, 1000 }, 0.2f, 1.0f)) {
		staticObjects.emplace_back(obj);
	}

	auto controller = std::make_unique<LocalPlayerController>();

	auto freecam = GetFreecam();
	auto worldcam = controller->camera;

	bodies.emplace_back(new Body(std::move(controller)));



	GraphicsEngine::Get().currentCamera = freecam;
	static auto c = Window::Get().postInputProccessing.Connect([freecam, worldcam](Window*) {
		if (Window::Get().PRESS_BEGAN_KEYS.contains(InputObject::F)) {
			if (GraphicsEngine::Get().currentCamera == worldcam) {
				GraphicsEngine::Get().currentCamera = freecam;
			}
			else {
				GraphicsEngine::Get().currentCamera = worldcam;
			}
		}
		else if (Window::Get().PRESS_BEGAN_KEYS.contains(InputObject::Space)) {
			Mainloop::Get().physicsPaused = !Mainloop::Get().physicsPaused;
		}
	});
}
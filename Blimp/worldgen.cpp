#include "game_state.hpp"
#include "debug_prefabs.hpp"
#include <window.hpp>
#include <mainloop.hpp>
#include "chat.hpp"

GameState::GameState() : chat()
{
}

void GameState::MakeGameplay() {
	chat.SetVisible(true);

	for (auto& obj : BuildPit({ 0, -10, 0 }, { 100, 10, 100 }, 0.2f, 1.0f)) {
		staticObjects.emplace_back(obj);
	}

	auto controller = std::make_unique<LocalPlayerController>();

	auto freecam = GetFreecam();
	auto worldcam = controller->camera;

	BodyCreateParams bp;
	bodies.emplace_back(new Body(std::move(controller), bp));
	bodies.back()->SetPos({ 0, -8, 0 });

	Mainloop::Get().physicsPaused = false;

	GraphicsEngine::Get().currentCamera = worldcam;
	static auto c = Window::Get().postInputProccessing.Connect([this, freecam, worldcam](Window*) {
		if (Window::Get().PRESS_BEGAN_KEYS.contains(InputType::F)) {
			if (GraphicsEngine::Get().currentCamera == worldcam) {
				GraphicsEngine::Get().currentCamera = freecam;
			}
			else {
				GraphicsEngine::Get().currentCamera = worldcam;
			}
		}
		else if (Window::Get().PRESS_BEGAN_KEYS.contains(InputType::Space)) {
			Mainloop::Get().physicsPaused = !Mainloop::Get().physicsPaused;
		}
		else if (Window::Get().PRESS_BEGAN_KEYS.contains(InputType::P)) {
			//DebugLogInfo("Sending " + std::to_string(NetworkingEngine::Get().IsClient()));
			//chat.SendMessage("Hello, world! client=" + std::to_string(NetworkingEngine::Get().IsClient()));
		}
	});
}
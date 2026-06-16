#include "game_state.hpp"
#include "debug_prefabs.hpp"

void GameState::MakeGameplay() {
	for (auto& obj : BuildPit({ 0, -10, 0 }, { 1000, 10, 1000 }, 0.2f, 1.0f)) {
		staticObjects.emplace_back(obj);
	}

	Freecam();

	bodies.emplace_back(new Body());
}
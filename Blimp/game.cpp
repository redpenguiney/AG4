#pragma once
#include "../Engine/debug_prefabs.hpp"
#include "game_state.hpp"
#include "game.hpp"
#include "event.hpp"
#include <vector>
#include <networking_engine.hpp>

Game::Game() {
	state = std::make_unique<GameState>();
	//state->MakeMainMenu();

	NetworkingEngine::Get().Host(13337);
}

Game::~Game() {
	state->menuEventConnections.clear();
	state->menuContainer = nullptr;
} 
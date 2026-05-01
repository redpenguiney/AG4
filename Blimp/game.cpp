#pragma once
#include "../Engine/debug_prefabs.hpp"
#include "game_state.hpp"
#include "game.hpp"
#include "event.hpp"
#include <vector>

Game::Game() {
	state->MakeMainMenu();
}

Game::~Game() {
	state->menuEventConnections.clear();
	state->menuContainer = nullptr;
} 
#pragma once
#include "../Engine/debug_prefabs.hpp"
#include "main_menu.hpp"


void GameMain() {
	MakeMainMenu();
}

void GameExit() {
	menuEventConnections.clear();
	menuContainer = nullptr;
} 
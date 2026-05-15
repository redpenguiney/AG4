#pragma once
#include <vector>
#include "game.hpp"
#include "event.hpp"

class GuiElement;

class GameState {
public:

	void MakeMainMenu();
	void MakeHostNewMenu();
	void MakeHostSavedMenu();
	void MakeJoinMenu();
	void MakeSettingsMenu();
	void MakeCreditsMenu();
	void MakeModsMenu();

	void MakeHostLoadingScreen();

	void MakeClientLoadingScreen();

	std::shared_ptr<GuiElement> menuContainer = nullptr;
	std::vector<Connection> menuEventConnections;
};
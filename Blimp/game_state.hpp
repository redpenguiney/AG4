#pragma once
#include "game.hpp"

class GameState {
public:
	class GuiElement;

	void MakeMainMenu();
	void MakeHostNewMenu();
	void MakeHostSavedMenu();
	void MakeJoinMenu();
	void MakeSettingsMenu();
	void MakeCreditsMenu();

	std::shared_ptr<GuiElement> menuContainer;
	std::vector<Connection> menuEventConnections;
};
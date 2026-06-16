#pragma once
#include <vector>
#include <memory>
#include "game.hpp"
#include "event.hpp"
#include "gameobject.hpp"
#include "body.hpp"

class GuiElement;

class GameState {
public:
	void MakeGameplay();


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

	std::vector<std::shared_ptr<Gameobject>> staticObjects;
	std::vector<std::shared_ptr<Gameobject>> dynamicObjects;
	std::vector<std::unique_ptr<Body>> bodies;
};
#pragma once
#include <vector>
#include <memory>
#include "game.hpp"
#include "event.hpp"
#include "gameobject.hpp"
#include "body.hpp"
#include "chat.hpp"
#include "gameplay_session.hpp"

class GuiElement;

class GameState {
public:
	GameState();

	Chat chat;

	void MakeGameplaySession();


	void MakeMainMenu();
	void MakeHostNewMenu();
	void MakeHostSavedMenu();
	void MakeJoinMenu();
	void MakeSettingsMenu();
	void MakeCreditsMenu();
	void MakeModsMenu();

	void MakeHostLoadingScreen();

	void MakeClientLoadingScreen();

	std::unique_ptr<GameplaySession> currentSession = nullptr;
	std::shared_ptr<GuiElement> menuContainer = nullptr;
	std::vector<Connection> menuEventConnections;
};
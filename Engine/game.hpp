#pragma once
#include <memory>

// Defined in a seperate project which defines the actual game.
class GameState;
// Implemented in a seperate project which defines the actual game.
class Game {
public:
	// Called when the game starts.
	Game();

	// Called when the game ends.
	~Game();

	// holds arbitrary globals.
	std::unique_ptr<GameState> state;
}
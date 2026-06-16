#pragma once
#include <memory>

// Defined in a seperate project which defines the actual game.
class GameState;
// Implemented in a seperate project which defines the actual game.
class Game {
public:
	// Called when the game starts.
	Game(std::vector<const char*> launchArgs);

	// Called when the game ends.
	~Game();

	// holds arbitrary globals.
	// You can also use local static variables instead; Game is itself only instantiated as one, so they are guaranteed to be destroyed before ~Game() runs.
	std::unique_ptr<GameState> state;
};
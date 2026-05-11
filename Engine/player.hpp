#pragma once

struct ConnectionInfo;

// Represents either a client connected to us or the local machine.
class Player {
public:
	Player(const Player&) = delete;
	Player(Player&&);
	~Player();

private:
	// Generates the player for the local machine.
	Player();

	// nullptr if local machine
	ConnectionInfo* connectionInfo;

	friend class NetworkingEngine;
};
#pragma once

class ConnectionInfo;

// Represents either a client connected to us or the local machine.
class Player {
public:
	Player(const Player&) = delete;

	friend class NetworkingEngine;

private:
	Player();
	~Player();
};
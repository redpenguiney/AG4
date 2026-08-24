#pragma once
#include <optional>
#include <GameNetworkingSockets/steam/steamnetworkingtypes.h>

// Represents either a client/server connected to us or the local machine.
// Mostly just an identifier for your use.
class Player {
public:
	Player(const Player&) = delete;
	~Player();

private:
	// nullopt if local machine
	std::optional<HSteamNetConnection> sockConnection;

	// Generates the player for the local machine.
	Player();

	// Generates the player for someone connected to us.
	Player(HSteamNetConnection);

	friend class NetworkingEngine;
	friend class Server;
	friend class Client;
};
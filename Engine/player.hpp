#pragma once
#include <optional>
#include <GameNetworkingSockets/steam/steamnetworkingtypes.h>

// Represents either a client/server connected to us or the local machine.
// On the server, players are only generated after a client trying to connect is successfully validated.
// Mostly just an identifier.
// Although you may hold onto a remote Player after it disconnects, it will never be reconnected; if some rejoins a server they will be assigned a new Player object every time they join.
class Player {
public:
	Player(const Player&) = delete;
	~Player();

	bool IsLocalMachine();

	// Equivalent to checking if the player is in NetworkingEngine::GetPlayers().
	// Returns true if local machine.
	bool IsConnected();
private:
	const size_t id; 

	// Generates the player for the local machine.
	Player();

	// Generates the player for someone connected to us.
	//Player(HSteamNetConnection);

	friend class NetworkingEngine;
	friend class Server;
	friend class Client;
};
#pragma once
#include <optional>
#include <GameNetworkingSockets/steam/steamnetworkingtypes.h>

// Represents either a client/server connected to us or the local machine.
// On the server, players are only generated after a client trying to connect is successfully validated.
// Mostly just an identifier.
// Although you may hold onto a remote Player after it disconnects, it will never be reconnected; if some rejoins a server they will be assigned a new Player object every time they join.
// Similarly, if the local machine disconnects from a server, a new Player object to represent the local machine will be created when it rejoins.
class Player {
public:
	Player(const Player&) = delete;
	~Player();

	// Will return false if the player WAS the local machine but has since disconnected.
	bool IsLocalMachine();

	// Equivalent to checking if the player is in NetworkingEngine::GetPlayers().
	// Returns true if local machine.
	bool IsConnected();

	const size_t id;
private:

	// Generates the player for the local machine.
	Player();

	// Generates the player for someone connected to us.
	//Player(HSteamNetConnection);

	friend class NetworkingEngine;
	friend class Server;
	friend class Client;
};
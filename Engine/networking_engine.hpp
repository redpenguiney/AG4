#pragma once
#include <variant>
#include <memory>
#include "event.hpp"
#include <GameNetworkingSockets/steam/steamnetworkingsockets.h>
#include <player.hpp>

enum class NetworkState {
	Offline,
	Server,
	ServerShuttingDown,
	ClientConnecting,
	Client,
	ClientDisconnecting
};

// Failures when initially attempting to connect to a server.
enum class ConnectionFailureReason: uint8_t {
	ConnectionRejectedByServer = 0,
	Unknown = 1
};

// Failures while already connected to a server.
enum class DisconnectionReason: uint8_t {
	TimedOut = 0,
	Unknown = 1
};

struct ServerNetworkInfo;
struct ClientNetworkInfo;

class NetworkingEngine {
public:
	const NetworkState GetState() const;
	const std::vector<Player>& GetPlayers() const;

	static NetworkingEngine& Get();
	void Update();

	// state must be Offline. Changes state to Server. Never fails.
	void Host(unsigned port);
	// state must be Offline. Change state to ServerShuttingDown. Wait for onNetworkStateChange to fire with new state Offline before terminating process or rehosting/etc.
	// Local process termination before this is complete is fine but will leave connected clients hanging until they timeout.
	void ShutdownServer();

	// state must be Offline. 
	// Changes state to ClientConnecting. Never fails. Will fire onNetworkStateChange when connection attempt fails or succeeds.
	// On success, state will be set to Client.
	// If it fails, state will be set back to Offline and it will also fire onConnectionFailure.
	void TryJoin();

	~NetworkingEngine();
	
	// returns prior state and new state in that order.
	static inline Event<NetworkingEngine, NetworkState, NetworkState>& onNetworkStateChange = Event<NetworkingEngine, NetworkState, NetworkState>::New();
	static inline Event<NetworkingEngine, ConnectionFailureReason, std::optional<std::string>>& onConnectionAttemptFailure = Event<NetworkingEngine, ConnectionFailureReason, std::optional<std::string>>::New();
private:
	// Size is always >0 and the first entry is always the local machine.
	std::vector<Player> players;

	void SetState(NetworkState newState);

	NetworkState state = NetworkState::Offline;
	// monostate for when offline
	std::variant<std::unique_ptr<ServerNetworkInfo>, std::unique_ptr<ClientNetworkInfo>, std::monostate> stateData = std::monostate();

	NetworkingEngine();
	NetworkingEngine(const NetworkingEngine&) = delete;

	void HandleRecievedMessage(Player& sender, SteamNetworkingMessage_t* message);
};

struct ServerNetworkInfo {
	HSteamListenSocket listenSocket;

	ServerNetworkInfo(unsigned port);

	~ServerNetworkInfo();
};

struct ClientNetworkInfo {

};

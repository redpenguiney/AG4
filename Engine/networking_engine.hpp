#pragma once
#include <variant>
#include <memory>
#include "event.hpp"
#include <GameNetworkingSockets/steam/steamnetworkingsockets.h>

enum class NetworkState {
	Offline,
	Server,
	ServerShuttingDown,
	ClientConnecting,
	Client,
	ClientDisconnecting
};

struct ServerNetworkInfo;
struct ClientNetworkInfo;

class NetworkingEngine {
public:
	const NetworkState GetState() const;

	static NetworkingEngine& Get();
	void Update();

	// state must be Offline. Changes state to Server. Never fails.
	void Host(unsigned port);
	// state must be Offline. Change state to ServerShuttingDown. Wait for onNetworkStateChange to fire with new state Offline before terminating process or rehosting/etc.
	void ShutdownServer();

	~NetworkingEngine();
	
	// returns prior state and new state in that order.
	static inline Event<NetworkingEngine, NetworkState, NetworkState>& onNetworkStateChange = Event<NetworkingEngine, NetworkState, NetworkState>::New();
private:
	void SetState(NetworkState newState);

	NetworkState state = NetworkState::Offline;
	// monostate for when offline
	std::variant<std::unique_ptr<ServerNetworkInfo>, std::unique_ptr<ClientNetworkInfo>, std::monostate> stateData = std::monostate();

	NetworkingEngine();
	NetworkingEngine(const NetworkingEngine&) = delete;
};

struct ServerNetworkInfo {
	HSteamListenSocket listenSocket;

	ServerNetworkInfo(unsigned port);

	~ServerNetworkInfo();
};

struct ClientNetworkInfo {

};

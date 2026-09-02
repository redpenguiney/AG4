#pragma once
#include <variant>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include "event.hpp"
#include <player.hpp>
#include <functional>
#include "serialization.hpp"
#include "log.hpp"
#include <tuple>
#include <GameNetworkingSockets/steam/steamnetworkingsockets.h>

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

class Server;
class Client;

struct ConnectionAttemptParams {
	std::string ip; // include port, defaults to 0 if not specified.
	int timeout; // in ms
};

struct HostServerParams {
	unsigned port = 13337;
};

enum class PacketType : uint8_t {
	UserMessage = 0, // 1 byte PacketType, 4 byte uint channel, then serialized data
};

class NetworkingEngine {
public:
	const NetworkState GetState() const;

	// if client, will only return local machine and server.
	// do not call if state is not client, server, or servershuttingdown.
	const std::vector<std::shared_ptr<Player>>& GetPlayers() const;

	// do not call if state is not client, server, or servershuttingdown.
	const std::shared_ptr<Player> GetLocalMachinePlayer() const;

	static NetworkingEngine& Get();
	void Update();

	// also returns true if in process of shutting down.
	bool IsHost();

	// also returns true if still connecting or disconnecting, be careful.
	bool IsClient();

	// client that can send data
	bool IsReadyClient();

	// state must be Offline. Changes state to Server. Never fails.
	// Note: connect all user channel events prior to connection completion to avoid missing early calls.
	void Host(HostServerParams params);
	
	// state must be Server or ServerShuttingDown (does nothing in the latter case). 
	// Changes state to ServerShuttingDown. Wait for onNetworkStateChange to fire with new state Offline before terminating process or rehosting/etc.
	// Local process termination before this is complete is fine but will leave connected clients hanging until they timeout.
	void ShutdownServer();

	// state must be Client or ClientDisconnecting (does nothing in the latter case). 
	// Changes state to ClientDisconnecting. Wait for onNetworkStateChange to fire with new state Offline before terminating process or calling TryJoin()/Host().
	// Local process termination before this is complete is fine but will leave server hanging until they timeout.
	void Disconnect();

	// state must be Offline. 
	// Changes state to ClientConnecting. Never fails. Will fire onNetworkStateChange when connection attempt fails or succeeds.
	// On success, state will be set to Client.
	// If it fails, state will be set back to Offline and it will also fire onConnectionFailure.
	// Note: connect all user channel events prior to connection completion to avoid missing early calls.
	void TryJoin(ConnectionAttemptParams params);
	// state must be ClientConnecting.
	void CancelJoin();

	~NetworkingEngine();

	// All given values must be serializable.
	// IsReadyClient() must be true.
	template <typename... Ts>
	void SendUserMessageToHost(unsigned channel, Ts...);

	// All given values must be serializable.
	// IsHost() must be true.
	// toWho must not be the local machine player.
	// May throw.
	template <typename... Ts>
	void SendUserMessageToClient(Player* toWho, unsigned channel, Ts... v);

	// All given values must be serializable.
	// IsHost() must be true.
	// May throw.
	template <typename... Ts>
	void SendUserMessageToAllClients(unsigned channel, Ts... v) {
		for (auto& p : GetPlayers()) {
			if (p != GetLocalMachinePlayer()) {
				SendUserMessageToClient(p.get(), channel, v...);
			}
		}
	}

	// TODO: bc stupid it deserializes the message seperately for each connection
	template <typename... Ts>
	void ConnectUserMessage(unsigned channel, std::function<void(Player*, Ts...)> callback) {
		if (!userEventChannels.contains(channel)) {
			userEventChannels[channel] = ReceivingChannelInfo{
				.channel = channel
			};
		}
		userEventChannels[channel].connected.push_back([callback, channel](Player* recFrom, uint8_t* data, size_t dataSize) {
			std::tuple<Ts...> deserialized;
			try {
				auto lambda = [&](auto& ...x) mutable {
					auto mutate = [&](auto& y) mutable {
						y = Deserialize<decltype(y)>(data, dataSize);
						};
					(..., mutate(x)); 
				};

				std::apply(lambda, deserialized);
			}
			catch (...) {
				DebugLogError("Couldn't deserialize recieved data on channel ", channel);
				return;
			}
			std::tuple<Player*, Ts...> args = std::tuple_cat(std::tuple<Player*>(recFrom), deserialized);
			std::apply(callback, args);
			});
	}
	
	// returns prior state and new state in that order.
	static inline Event<NetworkingEngine, NetworkState, NetworkState>& onNetworkStateChange = Event<NetworkingEngine, NetworkState, NetworkState>::New();
	static inline Event<NetworkingEngine, ConnectionFailureReason, std::optional<std::string>>& onConnectionAttemptFailure = Event<NetworkingEngine, ConnectionFailureReason, std::optional<std::string>>::New();

	// host only
	// fired immediately after player and connection objects are created
	static inline Event<NetworkingEngine, Player*>& onPlayerAdded = Event<NetworkingEngine, Player*>::New();

	// host only
	// fired immediately before player and connection objects are destroyed
	static inline Event<NetworkingEngine, Player*>& onPlayerRemoved = Event<NetworkingEngine, Player*>::New();
private:

	// first entry is always local machine
	std::vector<std::shared_ptr<Player>> players;

	// not populated for every player object
	std::unordered_map<Player*, ConnectionInfo*> playerConnections;

	struct ReceivingChannelInfo {
		unsigned channel;
		std::vector<std::function<void(uint8_t*, size_t)>> connected;
	};

	std::unordered_map<unsigned, ReceivingChannelInfo> userEventChannels;

	// Size is always >0 and the first entry is always the local machine.
	//std::vector<Player> players;

	void SetState(NetworkState newState);

	NetworkState state = NetworkState::Offline;
	// monostate for when offline
	std::variant<std::unique_ptr<Server>, std::unique_ptr<Client>, std::monostate> stateData = std::monostate();

	NetworkingEngine();
	NetworkingEngine(const NetworkingEngine&) = delete;

	static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo);

	friend class Server;
	friend class Client;

	void HandleUserMessage(uint8_t* data, int size);
};

struct ConnectionInfo {
	HSteamNetConnection connection;
	Player* connectee; // may be nullptr TODO

	ConnectionInfo(HSteamNetConnection conn);
	~ConnectionInfo();

};

class Server {
public:
	Server(HostServerParams params);
	~Server();

	void UpdateServer();
	void ShutdownServer();
private:
	HSteamListenSocket listenSocket;
	std::vector<std::unique_ptr<ConnectionInfo>> connections;

	void HandleRecievedMessage(SteamNetworkingMessage_t* msg);
	static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo);

	friend class NetworkingEngine; // only for the steam callback
};

class Client {
public:
	Client(ConnectionAttemptParams params);
	~Client();

	void UpdateClient();
	void Disconnect();
private:
	std::unique_ptr<ConnectionInfo> connection;

	void HandleRecievedMessage(SteamNetworkingMessage_t* msg);
	static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo);

	friend class NetworkingEngine; // only for the steam callback
};


// All given values must be serializable.
// IsReadyClient() must be true.
template<typename ...Ts>
inline void NetworkingEngine::SendUserMessageToHost(unsigned channel, Ts ... v) {
	Assert(IsReadyClient());

	size_t size = (SerializedSize(v) + ...) + SerializedSize((uint8_t)PacketType::UserMessage) + SerializedSize(channel);

	uint8_t* packet = reinterpret_cast<uint8_t*>(malloc(size));
	uint8_t* currentPos = packet;
	Serialize((uint8_t)PacketType::UserMessage, currentPos);
	Serialize(channel, currentPos);
	(..., Serialize(v, currentPos));

	std::unique_ptr<Client>& client = std::get<std::unique_ptr<Client>>(stateData);

	int flags = k_nSteamNetworkingSend_Reliable; // TODO options
	EResult sockError = SendMessageToConnection(client->connection->connection, packet, size, flags, nullptr);

	free(packet);

	if (sockError != EResult::k_EResultOK) {
		throw std::runtime_error("Sockets were mad! ", sockError);
	}
}

// All given values must be serializable.
// IsHost() must be true.
// toWho must not be the local machine player.
// May throw.
template<typename ...Ts>
inline void NetworkingEngine::SendUserMessageToClient(Player* toWho, unsigned channel, Ts ...v) {
	Assert(IsHost());
	Assert(toWho != GetLocalMachinePlayer().get());
	size_t size = (SerializedSize(v) + ...) + SerializedSize((uint8_t)PacketType::UserMessage) + SerializedSize(channel);

	uint8_t* packet = reinterpret_cast<uint8_t*>(malloc(size));
	uint8_t* currentPos = packet;
	Serialize((uint8_t)PacketType::UserMessage, currentPos);
	Serialize(channel, currentPos);
	(..., Serialize(v, currentPos));

	std::unique_ptr<Server>& server = std::get<std::unique_ptr<Server>>(stateData);
	
	int flags = k_nSteamNetworkingSend_Reliable; // TODO options
	EResult sockError = SendMessageToConnection(toWho->sockConnection, packet, size, flags, nullptr);

	free(packet);

	if (sockError != EResult::k_EResultOK) {
		throw std::runtime_error("Sockets were mad! ", sockError);
	}
}

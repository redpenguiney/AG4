#pragma once
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "gui.hpp"
#include <event.hpp>
#include <networking_engine.hpp>

class Player;

struct Message {
	std::string sender;
	std::string content;
};

// chat are we cooked
// instantiate only once, before doing anything with the network.
class Chat {
public:
	constexpr static unsigned CLIENT_TO_SERVER_USERNAME_IDENTIFICATION_CHANNEL = 1000;
	constexpr static unsigned CLIENT_TO_SERVER_MESSAGING_CHANNEL = 1001;
	constexpr static unsigned SERVER_TO_CLIENT_MESSAGING_CHANNEL = 1002;

	std::vector<Message> messages;

	// reposition as you see fit
	std::unique_ptr<GuiElement> ui;

	Chat();
	~Chat() = default;

	std::string GetUsername() const;
	void SetUsername(std::string name);

	Chat(const Chat&) = delete;

	void SendMessage(std::string message);
private:
	std::string ourUsername = "[you]";

	static void OnNetworkStateChange(Chat* c, NetworkState oldState, NetworkState newState);

	// server only, on client the server tells us the username for each message
	std::unordered_map<Player*, std::string> usernames;

	std::vector<Connection> messageConnections;
	Connection networkStateConnection;
};
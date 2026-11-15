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

	// oldest first
	std::vector<Message> messages;

	// reposition as you see fit
	std::shared_ptr<GuiElement> chatUiFrame;

	// don't call before BuildUi()
	// initially true
	void SetVisible(bool v);

	Chat();
	~Chat() = default;

	std::string GetUsername() const;
	void SetUsername(std::string name);

	Chat(const Chat&) = delete;

	void SendMessage(std::string message);

	// adds the given message to the local UI, but doesn't send it to other players.
	void LogMessage(std::string message);

	// fired immediately after network recieve, or immediately after SendMessage() is called IF we're the host and we called SendMessage().
	// WILL be fired for messages originally sent by the local machine.
		// though, for clients there will still be a delay since the message must be sent to the server and then back to the client before it is received.
	static inline Event<Chat, const Message&>& onMessageReceived = Event<Chat, const Message&>::New();
private:
	int nMessagesLogged = 0;

	bool chatVisible = true;

	void BuildUi();
	std::shared_ptr<GuiElement> GetMessageUiElement(const Message& m);

	std::string ourUsername = "[you]";

	static void OnNetworkStateChange(Chat* c, NetworkState oldState, NetworkState newState);

	void AddUiForMessage(std::string& message, std::string sender);

	// server only, on client the server tells us the username for each message
	std::unordered_map<Player*, std::string> usernames;

	std::vector<Connection> uiConnections;
	std::vector<Connection> messageConnections;
	Connection networkStateConnection;
	
	// nonowning, populated by BuildUi()
	TextboxElement* inputBox;
	ScrollableElement* messageList;
};
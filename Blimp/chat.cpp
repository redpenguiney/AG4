#include "chat.hpp"
#include "networking_engine.hpp"
#include <log.hpp>

Chat::Chat() {
	networkStateConnection = NetworkingEngine::Get().onNetworkStateChange.Connect([this](NetworkingEngine*, NetworkState old, NetworkState now) {
		OnNetworkStateChange(this, old, now);
	});
}

std::string Chat::GetUsername() const { 
	return ourUsername; 
}

void Chat::SetUsername(std::string name)
{
	ourUsername = name;

	if (NetworkingEngine::Get().IsReadyClient()) {
		NetworkingEngine::Get().SendUserMessageToHost(CLIENT_TO_SERVER_USERNAME_IDENTIFICATION_CHANNEL, ourUsername);
	}
}

void Chat::SendMessage(std::string message) {
	if (NetworkingEngine::Get().IsHost()) {
		messages.push_back({ ourUsername, message });
		for (auto& p : NetworkingEngine::Get().GetPlayers()) {
			if (p->IsLocalMachine()) continue;
			NetworkingEngine::Get().SendUserMessageToClient<std::string, std::string>(p.get(), SERVER_TO_CLIENT_MESSAGING_CHANNEL, ourUsername, message);
			DebugLogInfo("Sending to client w/ username ", usernames.contains(p.get()) ? usernames[p.get()] : std::string("UNKNOWN"));
		}
	}
	else if (NetworkingEngine::Get().IsReadyClient()) {
		messages.push_back({ ourUsername, message });
		NetworkingEngine::Get().SendUserMessageToHost<std::string>(CLIENT_TO_SERVER_MESSAGING_CHANNEL, message);
	}
	else {
		DebugLogError("Cannot send message while not connected to a server.");
	}
}

void Chat::OnNetworkStateChange(Chat* c, NetworkState old, NetworkState now)
{
	if (now == NetworkState::Offline) {
		c->usernames.clear();
		c->messageConnections.clear();
	}
	else if (now == NetworkState::Server) {
		c->messageConnections.push_back(NetworkingEngine::Get().onPlayerRemoved.Connect([c](NetworkingEngine*, Player* p) { 
			DebugLogInfo("PLAYER ", p->id, " DISCONNECTED");
			c->usernames.erase(p);
		}));

		c->messageConnections.push_back(NetworkingEngine::Get().ConnectUserMessage<std::string>(CLIENT_TO_SERVER_USERNAME_IDENTIFICATION_CHANNEL, std::function<void(Player*, std::string)>([c](Player* from, std::string username) {
			c->usernames[from] = username;
			DebugLogInfo("PLAYER ", from->id, " IDENTIFIED AS ", username);
		})));

		
		c->messageConnections.push_back(NetworkingEngine::Get().ConnectUserMessage<std::string>(CLIENT_TO_SERVER_MESSAGING_CHANNEL, std::function<void(Player*, std::string)>([c](Player* from, std::string message) {
			if (!c->usernames.contains(from)) {
				DebugLogInfo("PLAYER ", from->id, " WITH UNKNOWN USERNAME SENT MESSAGE: ", message);
				return;
			}
			c->messages.push_back({ c->usernames[from], message });

			// forward message to all other players
			for (auto& p : NetworkingEngine::Get().GetPlayers()) {
				if (p.get() == from || p->IsLocalMachine()) continue;
				NetworkingEngine::Get().SendUserMessageToClient<std::string, std::string>(p.get(), SERVER_TO_CLIENT_MESSAGING_CHANNEL, c->usernames[from], message);
			}

			DebugLogInfo("PLAYER ", from->id, " SENT MESSAGE: ", message);
			})));
	}
	else if (now == NetworkState::Client) {
		NetworkingEngine::Get().SendUserMessageToHost<std::string>(CLIENT_TO_SERVER_USERNAME_IDENTIFICATION_CHANNEL, c->ourUsername);

		c->messageConnections.push_back(NetworkingEngine::Get().ConnectUserMessage<std::string, std::string>(SERVER_TO_CLIENT_MESSAGING_CHANNEL, std::function<void(Player*, std::string, std::string)>([c](Player* from, std::string username, std::string message) {
			DebugLogInfo("Recieved message from ", username, ": ", message);
			c->messages.push_back({ username, message });
		})));
	}
}

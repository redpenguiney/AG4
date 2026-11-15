#include "chat_commands.hpp"
#include "chat.hpp"
#include "networking_engine.hpp"
#include <memory>



void LoadChatCommands(Chat& chat) {
	static std::vector<std::unique_ptr<IChatCommand>> commands;

	static auto c1 = chat.onMessageReceived.Connect([&chat](Chat*, const Message& message) {
		if (message.content.empty() || message.content[0] != '/') return;
		if (NetworkingEngine::Get().GetServerPlayer()->IsLocalMachine()) {
			for (auto& command : commands) {
				if (command->TryExecute(chat, message)) break;
			}
		}
		});
}

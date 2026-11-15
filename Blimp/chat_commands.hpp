#pragma once
#include "chat.hpp"

void LoadChatCommands(Chat& chat);

class IChatCommand {
public:
	// returns true if message invoked the command (even if incorrectly)
	virtual bool TryExecute(Chat& chat, const Message message) = 0;
};
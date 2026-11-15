#include "chat.hpp"
#include "networking_engine.hpp"
#include <log.hpp>
#include "ui_colors.hpp"

void Chat::SetVisible(bool v) {
	if (chatVisible == v) return;
	if (v) {
		chatUiFrame->pixelPosition -= glm::ivec2{200000, 0};
	}
	else {
		chatUiFrame->pixelPosition += glm::ivec2{200000, 0 };
	}
	chatUiFrame->RefreshTransform();
	chatVisible = v;
}

Chat::Chat() {
	networkStateConnection = NetworkingEngine::Get().onNetworkStateChange.Connect([this](NetworkingEngine*, NetworkState old, NetworkState now) {
		OnNetworkStateChange(this, old, now);
	});

	BuildUi();
	SetVisible(false);
}

std::string Chat::GetUsername() const { 
	return ourUsername; 
}

void Chat::SetUsername(std::string name)
{
	ourUsername = name;

	if (NetworkingEngine::Get().IsReadyClient()) {
		NetworkingEngine::Get().SendUserMessageToHost(CLIENT_TO_SERVER_USERNAME_IDENTIFICATION_CHANNEL, SendMessageParams(), ourUsername);
	}
}

void Chat::SendMessage(std::string message) {
	if (message.empty()) return;
	if (message.back() == '\n') message.pop_back();

	if (NetworkingEngine::Get().IsHost()) {
		messages.push_back({ ourUsername, message });
		for (auto& p : NetworkingEngine::Get().GetPlayers()) {
			if (p->IsLocalMachine()) continue;
			NetworkingEngine::Get().SendUserMessageToClient<std::string, std::string>(p.get(), SERVER_TO_CLIENT_MESSAGING_CHANNEL, SendMessageParams(), ourUsername, message);
			DebugLogInfo("Sending to client w/ username ", usernames.contains(p.get()) ? usernames[p.get()] : std::string("UNKNOWN"));
		}

		AddUiForMessage(message, ourUsername);
	}
	else if (NetworkingEngine::Get().IsReadyClient()) {
		//messages.push_back({ ourUsername, message }); message won't appear until server sends it back to us
		NetworkingEngine::Get().SendUserMessageToHost<std::string>(CLIENT_TO_SERVER_MESSAGING_CHANNEL, SendMessageParams(), message);
	}
	else {
		DebugLogError("Cannot send message while not connected to a server.");
	}
}

void Chat::LogMessage(std::string message) {
	AddUiForMessage(message, "[System]");
}

void Chat::BuildUi() {
	chatUiFrame = GuiElement::New(*GetScreenGuiContainer(), GuiCreateParams{ .clipChildren = false});
	chatUiFrame->backgroundColor = glm::vec4(UI_PREFABS::MAIN, 0.5f);
	chatUiFrame->anchorPoint = { -0.5, -0.5 };
	chatUiFrame->pixelPosition = { 8, 8 };
	chatUiFrame->pixelSize = { 400, 300 };
	chatUiFrame->RefreshGraphics();

	auto inputBoxPtr = TextboxElement::New(*GetScreenGuiContainer(), GuiCreateParams{ .font = UI_PREFABS::GetMenuFont<14>() });
	inputBoxPtr->SetParent(chatUiFrame.get());
	inputBox = inputBoxPtr.get();
	inputBox->anchorPoint = { -0.5, -0.5 };
	inputBox->percentageSize = { 1, 0 };
	inputBox->pixelSize = { -4, 30 };
	inputBox->pixelPosition = { 2, 2 };
	inputBox->SetEmptyText("");
	inputBox->clearOnFocus = false;
	inputBox->hAlign = HorizontalAlignMode::Left;
	chatUiFrame->RefreshTransform();

	uiConnections.push_back(inputBox->onTextChanged.Connect(inputBox, [this](TextboxElement*) {
		if (!inputBox->text.empty() && Window::Get().PRESS_BEGAN_KEYS.contains(InputType::Enter)) {
			this->SendMessage(inputBox->text);
			inputBox->ApplyText("");
		}
	}));

	auto messageBoxPtr = ScrollableElement::New(*GetScreenGuiContainer(), ScrollableGuiCreateParams{ .orientation = ScrollbarOrientation::Vertical });
	messageBoxPtr->SetParent(chatUiFrame.get());
	messageBoxPtr->depth = -10.0f;
	messageBoxPtr->anchorPoint = { -0.5, 0.5 };
	messageBoxPtr->percentageSize = { 1, 1 };
	messageBoxPtr->pixelSize = { -4, -38 };
	messageBoxPtr->percentagePosition = { 0, 1 };
	messageBoxPtr->pixelPosition = { 2, -2 };
	messageBoxPtr->backgroundColor = { 0, 0, 0, 0 };
	messageBoxPtr->canvas->backgroundColor = { 0, 0, 0, 0.5 };
	auto layout = std::make_unique<ListLayout>();
	layout->childPercentStride = { 0, 1 };
	messageBoxPtr->canvas->childrenLayout = std::move(layout);

	messageList = messageBoxPtr.get();

	messageBoxPtr->RefreshTransform();
	messageBoxPtr->RefreshGraphics();
}

std::shared_ptr<GuiElement> Chat::GetMessageUiElement(const Message& m) {
	auto element = GuiElement::New(*GetScreenGuiContainer(), GuiCreateParams{ .font = UI_PREFABS::GetMenuFont<14>() });
	element->sortOrder = -(nMessagesLogged++);
	element->text = m.sender + " said: \"" + m.content + "\"";
	element->percentageSize = { 1, 0 };
	element->pixelPosition = { 0, 0 };
	element->backgroundColor = { 0, 0, 0, 0 };
	element->textColor = glm::vec4(UI_PREFABS::CONTRAST, 1.0f);
	element->anchorPoint = { -0.5, -0.5 };
	element->pixelSize = { 0, 64 }; // TODO: method to get neccesary height/size to contain the wrapped text
	element->wrapText = true;
	element->hAlign = HorizontalAlignMode::Left;
	element->vAlign = VerticalAlignMode::Top; 
	element->depth = -20;
	element->RefreshTransform();
	element->RefreshText();
	element->pixelSize = { 0, element->TextHeight() };
	return element;
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

			c->AddUiForMessage(message, c->usernames[from]);

			onMessageReceived.FireNow(c, c->messages.back());

			// forward message to all other players
			for (auto& p : NetworkingEngine::Get().GetPlayers()) {
				if (/*p.get() == from ||*/ p->IsLocalMachine()) continue;
				NetworkingEngine::Get().SendUserMessageToClient<std::string, std::string>(p.get(), SERVER_TO_CLIENT_MESSAGING_CHANNEL, SendMessageParams(), c->usernames[from], message);
			}

			DebugLogInfo("PLAYER ", from->id, " SENT MESSAGE: ", message);
			})));
	}
	else if (now == NetworkState::Client) {
		NetworkingEngine::Get().SendUserMessageToHost<std::string>(CLIENT_TO_SERVER_USERNAME_IDENTIFICATION_CHANNEL, SendMessageParams(), c->ourUsername);

		c->messageConnections.push_back(NetworkingEngine::Get().ConnectUserMessage<std::string, std::string>(SERVER_TO_CLIENT_MESSAGING_CHANNEL, std::function<void(Player*, std::string, std::string)>([c](Player* from, std::string username, std::string message) {
			DebugLogInfo("Recieved message from ", username, ": ", message);
			c->messages.push_back({ username, message });

			c->AddUiForMessage(message, username);

			onMessageReceived.FireNow(c, c->messages.back());
		})));
	}
}

void Chat::AddUiForMessage(std::string& message, std::string sender)
{
	if (messageList && messageList->canvas) {
		auto element = GetMessageUiElement(Message{ .sender = sender, .content = message });
		element->SetParent(messageList->canvas.get());
		messageList->RefreshTransform();
	}
}

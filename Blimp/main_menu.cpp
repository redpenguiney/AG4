#include "gui.hpp"
#include "texture.hpp"
#include "window.hpp"
#include "game_state.hpp"
#include <networking_engine.hpp>

glm::vec3 MAIN_COLOR(0.75, 0.4, 0.9);
glm::vec3 DARK_COLOR(0.65, 0.3, 0.8);
glm::vec3 HIGHLIGHT_COLOR(0.95, 0.6, 1);
glm::vec3 CONTRAST_COLOR(1, 1, 1);

std::shared_ptr<Texture> GeoFont(unsigned size) {
	TextureCreateParams menuFontParams({ TextureSource("../fonts/Arial.ttf"), });
	menuFontParams.fontHeight = size;
	menuFontParams.format = Texture::Grayscale_8Bit;
	return std::make_shared<Texture>(menuFontParams, Texture::Texture2D);
}

template <unsigned fontSize>
std::shared_ptr<Texture>& GetMenuFont() {
	static auto font = GeoFont(fontSize);
	return font;
}

void GameState::MakeHostNewMenu() {
	menuContainer = nullptr;
	menuEventConnections.clear();

	menuContainer = GuiElement::New(*GetScreenGuiContainer());
	menuContainer->backgroundColor = glm::vec4(MAIN_COLOR, 1.0f);
	menuContainer->anchorPoint = { 0.0, 0.0 };
	menuContainer->percentagePosition = { 0.5, 0.5 };
	menuContainer->pixelPosition = { 0, 0 };
	menuContainer->pixelSize = { 450, 400 };
	menuContainer->RefreshGraphics();
	menuContainer->RefreshTransform();
	auto headerFont = GetMenuFont<24>();
	auto mainTextFont = GetMenuFont<18>();

	auto optionsContainer = GuiElement::New(*GetScreenGuiContainer());
	optionsContainer->SetParent(menuContainer.get());
	optionsContainer->backgroundColor.w = 0;
	optionsContainer->percentageSize = { 1, 0.8 };
	optionsContainer->percentagePosition = { 0.5, 1.0 };
	//optionsContainer->pixelPosition = {}
	optionsContainer->pixelSize = { -10, 0 };
	optionsContainer->anchorPoint = { 0, 0.5 };
	optionsContainer->RefreshGraphics();
	optionsContainer->RefreshTransform();

	std::vector <TextboxElement*> options;
	int nOption = 0;
	auto makeTextboxMenuOption = [optionsContainer, mainTextFont, &nOption, &options](std::string optionName, std::string defaultValue) {
		auto option = GuiElement::New(*GetScreenGuiContainer(), nullptr, nullptr);
		option->SetParent(optionsContainer.get());
		option->pixelSize = { 0, 30 };
		option->percentageSize = { 1.0, 0 };
		option->sortOrder = nOption++;
		option->anchorPoint = { -0.5, 0.5 };
		option->backgroundColor.w = 0;
		option->RefreshGraphics();
		option->RefreshTransform();

		auto label = GuiElement::New(*GetScreenGuiContainer(), nullptr, mainTextFont);
		label->SetParent(option.get());
		label->percentageSize = { 0.6, 1};
		label->pixelSize = { -2, -4 };
		label->percentagePosition = { 0, 0.5 };
		label->pixelPosition = { 0, 0 };
		label->anchorPoint = { -0.5, 0.0 };
		label->depth = optionsContainer->depth - 1;
		label->wrapText = false;
		label->hAlign = HorizontalAlignMode::Left;
		label->backgroundColor = glm::vec4(DARK_COLOR, 1.0f);
		label->textColor = glm::vec4(CONTRAST_COLOR, 1.0f);
		label->text = optionName;
		label->RefreshText();

		auto entryField = TextboxElement::New(*GetScreenGuiContainer(), nullptr, mainTextFont);
		entryField->SetParent(option.get());
		entryField->percentageSize = { 0.4, 1 };
		entryField->pixelSize = { -2, -4 };
		entryField->percentagePosition = { 1, 0.5 };
		entryField->pixelPosition = { 0, 0 };
		entryField->anchorPoint = { 0.5, 0.0 };
		entryField->depth = optionsContainer->depth - 1;
		entryField->wrapText = false;
		entryField->hAlign = HorizontalAlignMode::Right;
		entryField->backgroundColor = glm::vec4(0, 0, 0, 1.0f);
		entryField->textColor = glm::vec4(CONTRAST_COLOR, 1.0f);
		entryField->clearOnFocus = false;
		entryField->ApplyText(defaultValue);

		options.push_back(entryField.get());
	};

	makeTextboxMenuOption("Port:", "41337");
	makeTextboxMenuOption("Password:", "abc");
	makeTextboxMenuOption("Max players:", "9999");

	optionsContainer->LayoutChildrenList(ListLayout{
		.pixelStart = {0, -40},
		.percentStart = {0.0, 1.0},
		.pixelStride = {0, -30},
		});
	optionsContainer->RefreshTransform();

	auto buttonsContainer = GuiElement::New(*GetScreenGuiContainer());
	buttonsContainer->SetParent(menuContainer.get());
	buttonsContainer->backgroundColor = glm::vec4(1, 1, 1, 0);
	buttonsContainer->percentageSize = { 1, 0.2 };
	buttonsContainer->percentagePosition = { 0.5, 0.2 };
	buttonsContainer->pixelPosition = { 0, 5 };
	buttonsContainer->pixelSize = { -10, -5 };
	buttonsContainer->anchorPoint = { 0, 0.5 };
	buttonsContainer->RefreshGraphics();
	buttonsContainer->RefreshTransform();

	nOption = 0;
	auto makeButton = [this, buttonsContainer, mainTextFont, &nOption](std::string label, std::function<void()> onActivate) {
		auto butt = GuiElement::New(*GetScreenGuiContainer(), nullptr, mainTextFont);
		butt->SetParent(buttonsContainer.get());
		butt->percentageSize = { 0, 0 };
		butt->pixelSize = { 60, 30 };
		butt->percentagePosition = { 0.5, 0.5 };
		butt->pixelPosition = { 4 + nOption * 64, 0 };
		butt->anchorPoint = { -0.5, 0.0 };
		butt->depth = buttonsContainer->depth - 2;
		butt->wrapText = false;
		butt->hAlign = HorizontalAlignMode::Center;
		butt->backgroundColor = glm::vec4(DARK_COLOR, 1.0f);
		butt->textColor = glm::vec4(CONTRAST_COLOR, 1.0f);
		butt->text = label;
		butt->RefreshText();

		auto hOnConnection = butt->onHoverBegin.Connect(butt.get(), [this](GuiElement* elem) {
			elem->backgroundColor = glm::vec4(HIGHLIGHT_COLOR, 0.5f);
			elem->RefreshGraphics();
			Window::Get().UseCursor(Window::Get().systemSelectionCursor);
			});
		menuEventConnections.push_back(std::move(hOnConnection));
		auto hOffConnection = butt->onHoverEnd.Connect(butt.get(), [this](GuiElement* elem) {
			elem->backgroundColor = glm::vec4(DARK_COLOR, 0.5f);
			elem->RefreshGraphics();
			Window::Get().UseCursor(Window::Get().systemPointerCursor);
			});
		menuEventConnections.push_back(std::move(hOffConnection));
		auto clickConnection = butt->onInputEnd.Connect(butt.get(), [onActivate](GuiElement*, InputObject input) {
			if (input.input == InputType::LMB) onActivate();
			});
		menuEventConnections.push_back(std::move(clickConnection));

		nOption++;
	};

	makeButton("Cancel", [this]() {
		//menuEventConnections.clear();
		//menuContainer = nullptr;
		MakeMainMenu();
	});
	makeButton("Host", [this, options]() {
		int port = -1;
		try {
			char* endptr;
			port = std::strtol(options[0]->text.c_str(), &endptr, 10);
			if (*endptr != '\0') {
				port = -1;
			}
		}
		catch (...) {}

		if (port < 0 || port > 65535) {
			options[0]->ApplyText("Invalid port");
		}
		else {
			HostServerParams params;
			params.port = port;
			NetworkingEngine::Get().Host(params);

			MakeHostLoadingScreen();
		}
	});
}

void GameState::MakeHostSavedMenu() {
	menuContainer = nullptr;
	menuEventConnections.clear();
}

void GameState::MakeJoinMenu() {
	menuContainer = nullptr;
	menuEventConnections.clear();

	menuContainer = GuiElement::New(*GetScreenGuiContainer());
	menuContainer->backgroundColor = glm::vec4(MAIN_COLOR, 1.0f);
	menuContainer->anchorPoint = { 0.0, 0.0 };
	menuContainer->percentagePosition = { 0.5, 0.5 };
	menuContainer->pixelPosition = { 0, 0 };
	menuContainer->pixelSize = { 450, 400 };
	menuContainer->RefreshGraphics();
	menuContainer->RefreshTransform();
	auto headerFont = GetMenuFont<24>();
	auto mainTextFont = GetMenuFont<18>();

	auto optionsContainer = GuiElement::New(*GetScreenGuiContainer());
	optionsContainer->SetParent(menuContainer.get());
	optionsContainer->backgroundColor.w = 0;
	optionsContainer->percentageSize = { 1, 0.8 };
	optionsContainer->percentagePosition = { 0.5, 1.0 };
	//optionsContainer->pixelPosition = {}
	optionsContainer->pixelSize = { -10, 0 };
	optionsContainer->anchorPoint = { 0, 0.5 };
	optionsContainer->RefreshGraphics();
	optionsContainer->RefreshTransform();

	std::vector <TextboxElement*> options;
	int nOption = 0;
	auto makeTextboxMenuOption = [optionsContainer, mainTextFont, &nOption, &options](std::string optionName, std::string defaultValue) {
		auto option = GuiElement::New(*GetScreenGuiContainer(), nullptr, nullptr);
		option->SetParent(optionsContainer.get());
		option->pixelSize = { 0, 30 };
		option->percentageSize = { 1.0, 0 };
		option->sortOrder = nOption++;
		option->anchorPoint = { -0.5, 0.5 };
		option->backgroundColor.w = 0;
		option->RefreshGraphics();
		option->RefreshTransform();

		auto label = GuiElement::New(*GetScreenGuiContainer(), nullptr, mainTextFont);
		label->SetParent(option.get());
		label->percentageSize = { 0.6, 1 };
		label->pixelSize = { -2, -4 };
		label->percentagePosition = { 0, 0.5 };
		label->pixelPosition = { 0, 0 };
		label->anchorPoint = { -0.5, 0.0 };
		label->depth = optionsContainer->depth - 1;
		label->wrapText = false;
		label->hAlign = HorizontalAlignMode::Left;
		label->backgroundColor = glm::vec4(DARK_COLOR, 1.0f);
		label->textColor = glm::vec4(CONTRAST_COLOR, 1.0f);
		label->text = optionName;
		label->RefreshText();

		auto entryField = TextboxElement::New(*GetScreenGuiContainer(), nullptr, mainTextFont);
		entryField->SetParent(option.get());
		entryField->percentageSize = { 0.4, 1 };
		entryField->pixelSize = { -2, -4 };
		entryField->percentagePosition = { 1, 0.5 };
		entryField->pixelPosition = { 0, 0 };
		entryField->anchorPoint = { 0.5, 0.0 };
		entryField->depth = optionsContainer->depth - 1;
		entryField->wrapText = false;
		entryField->hAlign = HorizontalAlignMode::Right;
		entryField->backgroundColor = glm::vec4(0, 0, 0, 1.0f);
		entryField->textColor = glm::vec4(CONTRAST_COLOR, 1.0f);
		entryField->clearOnFocus = false;
		entryField->ApplyText(defaultValue);

		options.push_back(entryField.get());
		};

	makeTextboxMenuOption("IP:", "127.0.0.1:41337");
	makeTextboxMenuOption("Password:", "abc");

	optionsContainer->LayoutChildrenList(ListLayout{
		.pixelStart = {0, -40},
		.percentStart = {0.0, 1.0},
		.pixelStride = {0, -30},
		});
	optionsContainer->RefreshTransform();

	auto buttonsContainer = GuiElement::New(*GetScreenGuiContainer());
	buttonsContainer->SetParent(menuContainer.get());
	buttonsContainer->backgroundColor = glm::vec4(1, 1, 1, 0);
	buttonsContainer->percentageSize = { 1, 0.2 };
	buttonsContainer->percentagePosition = { 0.5, 0.2 };
	buttonsContainer->pixelPosition = { 0, 5 };
	buttonsContainer->pixelSize = { -10, -5 };
	buttonsContainer->anchorPoint = { 0, 0.5 };
	buttonsContainer->RefreshGraphics();
	buttonsContainer->RefreshTransform();

	nOption = 0;
	auto makeButton = [this, buttonsContainer, mainTextFont, &nOption](std::string label, std::function<void()> onActivate) {
		auto butt = GuiElement::New(*GetScreenGuiContainer(), nullptr, mainTextFont);
		butt->SetParent(buttonsContainer.get());
		butt->percentageSize = { 0, 0 };
		butt->pixelSize = { 80, 30 };
		butt->percentagePosition = { 0.5, 0.5 };
		butt->pixelPosition = { 4 + nOption * 84, 0 };
		butt->anchorPoint = { -0.5, 0.0 };
		butt->depth = buttonsContainer->depth - 2;
		butt->wrapText = false;
		butt->hAlign = HorizontalAlignMode::Center;
		butt->backgroundColor = glm::vec4(DARK_COLOR, 1.0f);
		butt->textColor = glm::vec4(CONTRAST_COLOR, 1.0f);
		butt->text = label;
		butt->RefreshText();

		auto hOnConnection = butt->onHoverBegin.Connect(butt.get(), [this](GuiElement* elem) {
			elem->backgroundColor = glm::vec4(HIGHLIGHT_COLOR, 0.5f);
			elem->RefreshGraphics();
			Window::Get().UseCursor(Window::Get().systemSelectionCursor);
			});
		menuEventConnections.push_back(std::move(hOnConnection));
		auto hOffConnection = butt->onHoverEnd.Connect(butt.get(), [this](GuiElement* elem) {
			elem->backgroundColor = glm::vec4(DARK_COLOR, 0.5f);
			elem->RefreshGraphics();
			Window::Get().UseCursor(Window::Get().systemPointerCursor);
			});
		menuEventConnections.push_back(std::move(hOffConnection));
		auto clickConnection = butt->onInputEnd.Connect(butt.get(), [onActivate](GuiElement*, InputObject input) {
			if (input.input == InputType::LMB) onActivate();
			});
		menuEventConnections.push_back(std::move(clickConnection));

		nOption++;
		};

	makeButton("Return", [this]() {
		//menuEventConnections.clear();
		//menuContainer = nullptr;
		MakeMainMenu();
		});
	makeButton("Connect", [this, options]() {
		NetworkingEngine::Get().TryJoin(ConnectionAttemptParams{
		.ip = options[0]->text,
			});
		MakeClientLoadingScreen();
		});

	
}

void GameState::MakeSettingsMenu() {
}

void GameState::MakeCreditsMenu() {
}

void GameState::MakeModsMenu() {
}

void GameState::MakeHostLoadingScreen() {
	menuContainer = nullptr;
	menuEventConnections.clear();

	MakeGameplay();
}

void GameState::MakeClientLoadingScreen() {
	menuContainer = nullptr;
	menuEventConnections.clear();

	menuContainer = GuiElement::New(*GetScreenGuiContainer());
	menuContainer->backgroundColor = glm::vec4(MAIN_COLOR, 1.0);
	menuContainer->anchorPoint = { 0.0, 0.0 };
	menuContainer->percentagePosition = { 0.5, 0.5 };
	menuContainer->pixelPosition = { 0, 0 };
	menuContainer->pixelSize = { 500, 200};
	menuContainer->RefreshGraphics();

	auto statusText = GuiElement::New(*GetScreenGuiContainer(), nullptr, GetMenuFont<18>());
	statusText->SetParent(menuContainer.get());
	statusText->backgroundColor.w = 0;
	statusText-> percentageSize = { 1, 1 };
	statusText->pixelSize = { -8, -64 };
	statusText->anchorPoint = { 0.0, 0.0 };
	statusText->percentagePosition = { 0.5, 0.5 };
	statusText->text = "Connecting to server...";
	statusText->textColor = glm::vec4(CONTRAST_COLOR, 1.0f);
	statusText->hAlign = HorizontalAlignMode::Center;
	statusText->vAlign = VerticalAlignMode::Center;
	statusText->wrapText = true;
	statusText->RefreshText();

	auto cancelButton = GuiElement::New(*GetScreenGuiContainer(), nullptr, GetMenuFont<18>());
	cancelButton->SetParent(menuContainer.get());
	cancelButton->backgroundColor = glm::vec4(DARK_COLOR, 1.0f);
	cancelButton->percentageSize = { 0, 0 };
	cancelButton->pixelSize = { 80, 30 };
	cancelButton->percentagePosition = { 0.5, 0 };
	cancelButton->pixelPosition = { 0, 4 };
	cancelButton->anchorPoint = { 0.0, -0.5 };
	cancelButton->depth = menuContainer->depth - 1;
	cancelButton->wrapText = false;
	cancelButton->hAlign = HorizontalAlignMode::Center;
	cancelButton->textColor = glm::vec4(CONTRAST_COLOR, 1.0f);
	cancelButton->text = "Cancel";
	cancelButton->RefreshText();
	auto hOnConnection = cancelButton->onHoverBegin.Connect(cancelButton.get(), [this](GuiElement* elem) {
		elem->backgroundColor = glm::vec4(HIGHLIGHT_COLOR, 0.5f);
		elem->RefreshGraphics();
		Window::Get().UseCursor(Window::Get().systemSelectionCursor);
		});
	menuEventConnections.push_back(std::move(hOnConnection));
	auto hOffConnection = cancelButton->onHoverEnd.Connect(cancelButton.get(), [this](GuiElement* elem) {
		elem->backgroundColor = glm::vec4(DARK_COLOR, 0.5f);
		elem->RefreshGraphics();
		Window::Get().UseCursor(Window::Get().systemPointerCursor);
		});
	menuEventConnections.push_back(std::move(hOffConnection));
	auto clickConnection = cancelButton->onInputEnd.Connect(cancelButton.get(), [this](GuiElement*, InputObject input) {
		if (input.input == InputType::LMB) {
			if (NetworkingEngine::Get().GetState() == NetworkState::ClientConnecting) NetworkingEngine::Get().CancelJoin();
			MakeJoinMenu();
		}
		});
	menuEventConnections.push_back(std::move(clickConnection));
	menuContainer->RefreshTransform();

	auto status = statusText.get();
	auto failconn = NetworkingEngine::Get().onConnectionAttemptFailure.Connect([status](NetworkingEngine*, ConnectionFailureReason reason, std::optional<std::string> message) {
		switch (reason) {
			case ConnectionFailureReason::ConnectionRejectedByServer:
				status->text = "Connection rejected by server:\n" + message.value_or("no further information.");
				status->RefreshText();
				break;
			case ConnectionFailureReason::Unknown:
				[[fallthrough]];
			default:
				status->text = "Connection attempt failed:\n" + message.value_or("no further information.");
				status->RefreshText();
				break;
		}
		});
	menuEventConnections.push_back(std::move(failconn));

	auto connChange = NetworkingEngine::Get().onNetworkStateChange.Connect([this, status](NetworkingEngine*, NetworkState old, NetworkState now) {
		if (old == NetworkState::ClientConnecting && now == NetworkState::Client) {
			menuContainer = nullptr;
			menuEventConnections.clear();
		}
		});
	menuEventConnections.push_back(std::move(connChange));
}

void GameState::MakeMainMenu() {
	menuContainer = nullptr;
	menuEventConnections.clear();

	menuContainer = GuiElement::New(*GetScreenGuiContainer());
	menuContainer->backgroundColor.w = 0.0;
	menuContainer->anchorPoint = { -0.5, -0.5 };
	menuContainer->percentagePosition = { 0, 0 };
	menuContainer->pixelPosition = { 50, 50 };
	menuContainer->pixelSize = { 250, 350 };
	menuContainer->RefreshGraphics();

	auto menuFont = GetMenuFont<30>();

	auto makeMenuButton = [this, menuFont](std::string label, int order, std::function<void()> onActivate) {
		auto butt = GuiElement::New(*GetScreenGuiContainer(), nullptr, menuFont);
		butt->SetParent(menuContainer.get());
		butt->anchorPoint = { -0.5, 0.5};
		butt->percentageSize = { 1, 0 };
		butt->pixelSize = { 0, 30 };
		butt->wrapText = false;
		butt->hAlign = HorizontalAlignMode::Left;
		butt->backgroundColor = glm::vec4(MAIN_COLOR, 0.5f);
		butt->textColor = glm::vec4(CONTRAST_COLOR, 1.0f);
		butt->text = label;
		butt->sortOrder = order;
		butt->RefreshText();

		auto hOnConnection = butt->onHoverBegin.Connect(butt.get(), [this](GuiElement* elem) {
			elem->backgroundColor = glm::vec4(HIGHLIGHT_COLOR, 0.5f);
			elem->RefreshGraphics();
			Window::Get().UseCursor(Window::Get().systemSelectionCursor);
			}); 
		menuEventConnections.push_back(std::move(hOnConnection));
		auto hOffConnection = butt->onHoverEnd.Connect(butt.get(), [this](GuiElement* elem) {
			elem->backgroundColor = glm::vec4(MAIN_COLOR, 0.5f);
			elem->RefreshGraphics();
			Window::Get().UseCursor(Window::Get().systemPointerCursor);
			});
		menuEventConnections.push_back(std::move(hOffConnection));
		auto clickConnection = butt->onInputEnd.Connect(butt.get(), [onActivate](GuiElement*, InputObject input) {
			if (input.input == InputType::LMB) onActivate();
			});
		menuEventConnections.push_back(std::move(clickConnection));
		};

	makeMenuButton("Host new game", 0, [this]() {MakeHostNewMenu(); });
	makeMenuButton("Host saved game", 1, [this]() {MakeHostSavedMenu(); });
	makeMenuButton("Join game", 2, [this]() {MakeJoinMenu(); });
	makeMenuButton("Credits", 5, [this]() {MakeCreditsMenu();  });
	makeMenuButton("Settings", 4, [this]() {MakeSettingsMenu(); });
	makeMenuButton("Mods", 3, [this]() {MakeModsMenu(); });
	makeMenuButton("Quit", 6, []() {
		Window::Get().Close();
		});


	menuContainer->LayoutChildrenList(ListLayout{
		.percentStart = {0.0, 1.0},
		.pixelStride = {0, -50},
		});
	menuContainer->RefreshTransform();
}

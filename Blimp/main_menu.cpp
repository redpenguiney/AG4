#include "main_menu.hpp"
#include "gui.hpp"
#include "texture.hpp"
#include "window.hpp"
#include "game_state.hpp"

glm::vec3 MAIN_COLOR(0.75, 0.4, 0.9);
glm::vec3 DARK_COLOR(0.65, 0.3, 0.8);
glm::vec3 HIGHLIGHT_COLOR(0.95, 0.6, 1);
glm::vec3 CONTRAST_COLOR(1, 1, 1);

std::shared_ptr<Texture> GeoFont(unsigned size) {
	TextureCreateParams menuFontParams({ TextureSource("../fonts/Geo-Regular.ttf"), });
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

	auto buttonsContainer = GuiElement::New(*GetScreenGuiContainer());
	buttonsContainer->SetParent(menuContainer.get());
	buttonsContainer->backgroundColor.w = 0;
	buttonsContainer->percentageSize = { 1, 1 };
	buttonsContainer->percentagePosition = { 0.5, 0.5 };
	buttonsContainer->pixelSize = { -10, 0 };
	buttonsContainer->anchorPoint = { 0, 0 };
	buttonsContainer->RefreshGraphics();
	buttonsContainer->RefreshTransform();

	int nOption = 0;
	auto makeTextboxMenuOption = [buttonsContainer, mainTextFont, &nOption](std::string optionName, std::string defaultValue) {
		auto option = GuiElement::New(*GetScreenGuiContainer(), nullptr, nullptr);
		option->SetParent(buttonsContainer.get());
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
		label->depth = buttonsContainer->depth - 1;
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
		entryField->depth = buttonsContainer->depth - 1;
		entryField->wrapText = false;
		entryField->hAlign = HorizontalAlignMode::Right;
		entryField->backgroundColor = glm::vec4(0, 0, 0, 1.0f);
		entryField->textColor = glm::vec4(CONTRAST_COLOR, 1.0f);
		entryField->clearOnFocus = false;
		entryField->ApplyText(defaultValue);
	};

	makeTextboxMenuOption("Port:", "41337");
	makeTextboxMenuOption("Password:", "abc");
	makeTextboxMenuOption("Max players:", "9999");

	buttonsContainer->LayoutChildrenList(ListLayout{
		.pixelStart = {0, -40},
		.percentStart = {0.0, 1.0},
		.pixelStride = {0, -30},
		});
	buttonsContainer->RefreshTransform();
}

void GameState::MakeHostSavedMenu() {
	menuContainer = nullptr;
	menuEventConnections.clear();
}

void GameState::MakeJoinMenu() {
	menuContainer = nullptr;
	menuEventConnections.clear();
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
			if (input.input == InputObject::LMB) onActivate();
			});
		menuEventConnections.push_back(std::move(clickConnection));
		};

	makeMenuButton("Host new game", 0, MakeHostNewMenu);
	makeMenuButton("Host saved game", 1, MakeHostNewMenu);
	makeMenuButton("Join game", 2, MakeJoinMenu);
	makeMenuButton("Credits", 5, []() {});
	makeMenuButton("Settings", 4, []() {});
	makeMenuButton("Mods", 3, []() {});
	makeMenuButton("Quit", 6, []() {
		Window::Get().Close();
		});


	menuContainer->LayoutChildrenList(ListLayout{
		.percentStart = {0.0, 1.0},
		.pixelStride = {0, -50},
		});
	menuContainer->RefreshTransform();
}

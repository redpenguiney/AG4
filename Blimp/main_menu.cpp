#include "main_menu.hpp"
#include "gui.hpp"
#include "texture.hpp"
#include "window.hpp"

glm::vec3 MAIN_COLOR(0.75, 0.4, 0.9);
glm::vec3 HIGHLIGHT_COLOR(0.95, 0.6, 1);
glm::vec3 CONTRAST_COLOR(1, 1, 1);

void MakeHostNewMenu() {
	menuContainer = nullptr;
	menuEventConnections.clear();
}

void MakeHostSavedMenu() {
	menuContainer = nullptr;
	menuEventConnections.clear();
}

void MakeJoinMenu() {
	menuContainer = nullptr;
	menuEventConnections.clear();
}

void MakeMainMenu() {
	menuContainer = nullptr;
	menuEventConnections.clear();

	menuContainer = GuiElement::New(*GetScreenGuiContainer());
	menuContainer->backgroundColor.w = 0.0;
	menuContainer->anchorPoint = { -0.5, -0.5 };
	menuContainer->percentagePosition = { 0, 0 };
	menuContainer->pixelPosition = { 50, 50 };
	menuContainer->pixelSize = { 250, 350 };
	menuContainer->RefreshGraphics();

	TextureCreateParams menuFontParams({ TextureSource("../fonts/Geo-Regular.ttf"), });
	menuFontParams.fontHeight = 30;
	menuFontParams.format = Texture::Grayscale_8Bit;
	auto menuFont = std::make_shared<Texture>(menuFontParams, Texture::Texture2D);

	auto makeMenuButton = [menuFont](std::string label, int order, std::function<void()> onActivate) {
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

		auto hOnConnection = butt->onHoverBegin.Connect(butt.get(), [](GuiElement* elem) {
			elem->backgroundColor = glm::vec4(HIGHLIGHT_COLOR, 0.5f);
			elem->RefreshGraphics();
			Window::Get().UseCursor(Window::Get().systemSelectionCursor);
			});
		menuEventConnections.push_back(std::move(hOnConnection));
		auto hOffConnection = butt->onHoverEnd.Connect(butt.get(), [](GuiElement* elem) {
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

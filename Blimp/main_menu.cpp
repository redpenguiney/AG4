#include "main_menu.hpp"
#include "../Engine/gui.hpp"
#include "../Engine/texture.hpp"
#include "../Engine/window.hpp"

glm::vec3 MAIN_COLOR(0.75, 0.4, 0.9);
glm::vec3 HIGHLIGHT_COLOR(0.95, 0.6, 1);
glm::vec3 CONTRAST_COLOR(1, 1, 1);

void MakeMainMenu() {
	mainMenuContainer = GuiElement::New(*GetScreenGuiContainer());
	mainMenuContainer->backgroundColor.w = 0.0;
	mainMenuContainer->anchorPoint = { -0.5, -0.5 };
	mainMenuContainer->percentagePosition = { 0, 0 };
	mainMenuContainer->pixelPosition = { 50, 50 };
	mainMenuContainer->pixelSize = { 250, 350 };
	mainMenuContainer->RefreshGraphics();

	TextureCreateParams menuFontParams({ TextureSource("../fonts/Geo-Regular.ttf"), });
	menuFontParams.fontHeight = 30;
	menuFontParams.format = Texture::Grayscale_8Bit;
	auto menuFont = std::make_shared<Texture>(menuFontParams, Texture::Texture2D);

	auto makeMenuButton = [menuFont](std::string label, int order, std::function<void()> onActivate) {
		auto butt = GuiElement::New(*GetScreenGuiContainer(), nullptr, menuFont);
		butt->SetParent(mainMenuContainer.get());
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

		
		};

	makeMenuButton("Host new game", 0, []() {});
	makeMenuButton("Host saved game", 1, []() {});
	makeMenuButton("Join game", 2, []() {});
	makeMenuButton("Credits", 5, []() {});
	makeMenuButton("Settings", 4, []() {});
	makeMenuButton("Mods", 3, []() {});
	makeMenuButton("Quit", 6, []() {
		Window::Get().Close();
		});


	mainMenuContainer->LayoutChildrenList(ListLayout{
		.percentStart = {0.0, 1.0},
		.pixelStride = {0, -50},
		});
	mainMenuContainer->RefreshTransform();
}

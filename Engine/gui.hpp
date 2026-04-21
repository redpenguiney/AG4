#pragma once
#include <vector>
#include <memory>
#include <glm/vec2.hpp>

class Gameobject;
class Texture;
class DrawPass;

class GuiElement;

class GuiContainer {
public:
private:
	// flat array of all GuiElements, including their descendants
	std::vector<std::shared_ptr<GuiElement>> contents;
	std::shared_ptr<DrawPass> pass;
	static std::vector<std::shared_ptr<GuiContainer>> containers;

	friend class GuiElement;

	friend std::shared_ptr<GuiContainer> ScreenGui();
};

// Returns the GuiContainer for normal stuff that's just getting rendered directly to the screen.
std::shared_ptr<GuiContainer> GetScreenGuiContainer();

class GuiElement {
public:
	std::shared_ptr<GuiElement> New(GuiContainer& storeIn = *GetScreenGuiContainer());

	~GuiElement();

	// Call to apply transform/hierarchy changes.
	void RefreshTransform();
	// Call to apply color/texture/etc. changes.
	void RefreshGraphics();
	// Call to apply text changes.
	void RefreshText();

	float rotation;
	float depth;
	glm::vec2 pixelPosition;
	glm::vec2 percentagePosition;
	glm::vec2 pixelSize;
	glm::vec2 percentageSize;
	glm::vec2 anchorPoint = { 0.0f, 0.0f }; // (0.5, 0.5) is top right corner, (-0.5, -0.5) is bottom left

	GuiElement* parent;
	std::vector<std::shared_ptr<GuiElement>> children;

	std::shared_ptr<Texture> font;

private:
	GuiElement(GuiContainer& storeIn);

	Gameobject* gameobject;
};





// TODO: 3d guis (billboard, surface)
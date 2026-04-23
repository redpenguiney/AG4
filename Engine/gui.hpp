#pragma once
#include <vector>
#include <memory>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <unordered_map>
#include "mesh_provider.hpp"

class Gameobject;
class Texture;
class DrawPass;

class GuiElement;

class GuiContainer {
public:
	~GuiContainer() = default;
private:
	// flat array of all GuiElements, including their descendants
	std::vector<std::shared_ptr<GuiElement>> contents;
	//std::shared_ptr<DrawPass> pass;
	std::unordered_map<Texture*, std::shared_ptr<DrawPass>> elementPasses;
	static std::vector<std::shared_ptr<GuiContainer>>& Containers();

	friend class GuiElement;

	friend std::shared_ptr<GuiContainer> ScreenGui();
};

// Returns the GuiContainer for normal stuff that's just getting rendered directly to the screen.
std::shared_ptr<GuiContainer> GetScreenGuiContainer();

class GuiElement: public std::enable_shared_from_this<GuiElement> {
public:
	static std::shared_ptr<GuiElement> New(GuiContainer& storeIn, std::shared_ptr<Texture> background = nullptr, std::shared_ptr<Texture> font = nullptr);

	~GuiElement();

	// Call to apply transform/hierarchy changes.
	// Recursively calls RefreshTransform() on the GuiElement's children.
	void RefreshTransform();
	// Call to apply color/etc. changes.
	void RefreshGraphics();
	// Call to apply text changes.
	void RefreshText();

	// in radians
	float rotation = 0.0f;


	float depth = 0.0f;

	glm::ivec2 pixelPosition = { 0, 0 };
	glm::vec2 percentagePosition = { 0.0, 0.0 };
	glm::ivec2 pixelSize = { 100, 100 };
	glm::vec2 percentageSize = { 0.0, 0.0 };
	glm::vec2 anchorPoint = { 0.0f, 0.0f }; // (0.5, 0.5) is top right corner, (-0.5, -0.5) is bottom left

	glm::vec4 backgroundColor = { 1.0, 1.0, 1.0, 1.0 };

	glm::vec4 textColor = { 0.0, 0.0, 0.0, 1.0 };
	std::string text = "DEFAULT TEXT";
	HorizontalAlignMode hAlign = HorizontalAlignMode::Center;
	VerticalAlignMode vAlign = VerticalAlignMode::Center;
	bool wrapText = true;
	// Unsafe if not calling through a std::shared_ptr<GuiElement>.
	void SetParent(GuiElement* newParent);

	glm::ivec2 GetPixelSize();
	glm::ivec2 GetPixelCenterPosition();

private:
	void MakeTextobject();

	GuiElement(GuiContainer& storeIn, std::shared_ptr<Texture> background, std::shared_ptr<Texture> font);

	std::shared_ptr<Texture> texture; // nullptr if no texture
	std::shared_ptr<Texture> font; // nullptr if no text

	std::unique_ptr<Gameobject> gameobject;
	std::unique_ptr<Gameobject> textobject; // may be nullptr
	GuiContainer* container; // non-owning, always valid

	GuiElement* parent;
	std::vector<std::shared_ptr<GuiElement>> children;

	// unparents the GuiElement, if it has a parent.
	void Orphan();
};





// TODO: 3d guis (billboard, surface)
#pragma once
#include <vector>
#include <memory>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <unordered_map>
#include "mesh_provider.hpp"
#include "event.hpp"
#include "window.hpp"

class Gameobject;
class Texture;
class DrawPass;

class GuiElement;
class TextboxElement;

class GuiContainer {
public:
	~GuiContainer() = default;
private:
	// flat array of all GuiElements, including their descendants
	//std::vector<GuiElement*> contents;
	//std::shared_ptr<DrawPass> pass;
	std::unordered_map<Texture*, std::shared_ptr<DrawPass>> elementPasses;
	static std::vector<std::shared_ptr<GuiContainer>>& Containers();

	friend class GuiElement;

	friend std::shared_ptr<GuiContainer> ScreenGui();
};

// Returns the GuiContainer for normal stuff that's just getting rendered directly to the screen.
std::shared_ptr<GuiContainer> GetScreenGuiContainer();

// percents are with respect to size of object LayoutChildrenList() is being called on.
// pixels are still absolute
struct ListLayout {
	glm::ivec2 pixelStart = { 0, 0 };
	glm::vec2 percentStart = { 0, 0 };
	glm::ivec2 pixelStride = { 0, 0 };
	glm::vec2 percentStride = { 0, 0 };
};

class GuiElement: public std::enable_shared_from_this<GuiElement> {
public:
	static inline std::vector<GuiElement*> elementsList;
	static inline std::vector<GuiElement*> elementsBeingHoveredOn;
	static void InitGuiEvents();
	static std::shared_ptr<GuiElement> New(GuiContainer& storeIn, std::shared_ptr<Texture> background = nullptr, std::shared_ptr<Texture> font = nullptr);

	virtual ~GuiElement();

	// Call to apply transform/hierarchy changes.
	// Recursively calls RefreshTransform() on the GuiElement's children.
	void RefreshTransform();
	// Call to apply color/etc. changes.
	void RefreshGraphics();
	// Call to apply text changes.
	// You also need to call this if the ui changes size and you want wrapped text to adjust.
	void RefreshText();

	// Sets position of children and calls RefreshTransform() on them.
	void LayoutChildrenList(ListLayout params);

	// in radians. Only use for elements without children that don't need to use the input/hover events (TODO ADD SUPPORT FOR THAT)
	float rotation = 0.0f;

	// used by LayoutChildren____() functions.
	int sortOrder = 0;

	// Smaller/more negative depth shows up on top. Be wary of increments <= 0.001 since that is the increment used to put text in front of its respective element.
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

	bool hover = false;

	static inline Event<GuiElement>& onHoverBegin = Event<GuiElement>::New();;
	static inline Event<GuiElement>& onHoverEnd = Event<GuiElement>::New();;
	static inline Event<GuiElement, InputObject>& onInputBegin = Event<GuiElement, InputObject>::New();
	static inline Event<GuiElement, InputObject>& onInputEnd = Event<GuiElement, InputObject>::New();

protected:
	static inline TextboxElement* currentlyFocused = nullptr;

	GuiElement(GuiContainer& storeIn, std::shared_ptr<Texture> background, std::shared_ptr<Texture> font);

private:
	glm::ivec2 GetParentBounds();
	glm::ivec2 GetParentOffset();

	void MakeTextobject();

	GuiElement(const GuiElement&) = delete;

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

class TextboxElement : public GuiElement {
public:
	// background may be nullptr, font is mandatory
	static std::shared_ptr<TextboxElement> New(GuiContainer& storeIn, std::shared_ptr<Texture> background, std::shared_ptr<Texture> font);

	bool clearOnFocus = true;

	// Does not require calling RefreshText().
	void SetEmptyText(std::string);

	// Different from text.empty() since text will contain emptyText when the user has not typed anything and the ui is unfocused.
	bool IsEmpty();

	virtual ~TextboxElement();

	// Makes the textbox element have the provided text as if a user typed it in. (just setting text would cause it to be overwritten due to the isEmpty flag not being updated)
	// Also calls RefreshText().
	void ApplyText(std::string txt);

	void Focus();
	void Unfocus();
private:
	TextboxElement(GuiContainer& storeIn, std::shared_ptr<Texture> background, std::shared_ptr<Texture> font);
	bool isEmpty = true;
	std::string emptyText = "type here";

	friend class GuiElement;
};



// TODO: 3d guis (billboard, surface)
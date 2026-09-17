#pragma once
#include <optional>
#include <glm/vec2.hpp>
#include <string>
#include "event.hpp"

enum class InputType : int {
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    One,
    Two,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    Zero,

    Minus,
    Equals,

    BackSlash, // (it's "\")
    ForwardSlash, // (it's "/")

    Space,
    Tab,
    Escape,
    Grave,
    LBracket,
    RBracket,

    Ctrl,
    Shift,
    Alt,

    LMB,// mouse button 1
    RMB,// mouse button 2
    MMB,// mouse button 3
    MB4,
    MB5,
    MB6,
    MB7,
    MB8,

    Scroll, // Note that capitalized is always false for Scroll events due to glfw/laziness limitations.

    LeftArrow,
    RightArrow,
    UpArrow,
    DownArrow,

    Backspace,

    Enter, // converted to newline by InputToString()

    Period,
    Comma,
    Semicolon,
    Apostrophe,

    Unknown,
};


enum class InputState {
	Down, // also used for scroll events
	Up,
    Held, // like when you hold a key and after a moment it just spammmmmmmmmmmmmms it; only logical for text entry

}; 

// Object representing any kind of input.
class InputObject {
public:

    InputState state;
    InputType input;

    glm::vec2 direction = { 0, 0 }; // has magnitude, used for scrolling/joysticks if i ever support them (i won't)/etc.

    bool capitalized; // if it's a letter, whether it's capitalized, accounting for both shift and capslock. Always false if input == Scroll, sorry. (TODO fix) (MORE RECENT TODO why is that an issue???)
    bool altDown;
    bool ctrlDown;
    bool shiftDown;

    bool operator== (const InputObject&) const = default;
};

std::optional<std::string> InputToString(InputObject input);

// hash InputObject and InputType so they can go in unordered map
// todo: add direction member to hash function
template <>
struct std::hash<InputObject> {
    std::size_t operator()(const InputObject& io) const noexcept {
        size_t h1 = io.capitalized + io.altDown * 8 + io.ctrlDown * 256 + io.shiftDown * 2048;
        size_t h2 = static_cast<size_t>(io.input);
        return h1 ^ (h2 << 1);
    }
};
template <>
struct std::hash<InputType> {
    std::size_t operator()(const InputType& io) const noexcept {
        return static_cast<size_t>(io);
    }
};

// RAII object representing a context for handling input events. When destroyed, it will remove itself from the input stack, disconnecting whatever was connected to it.
class InputContext {
public:
    InputContext(const InputContext&) = delete;
    ~InputContext();
    
    const int priority;

    // calls onInput on the next-highest priority InputContext.
    // Use when the original InputContext's handler decides it should not handle the input (like when gui is unfocused or smtg idk) but you didn't pop the context for some reason.
    void Fallthrough(InputObject);
private:
	friend class InputStack;
    InputContext(int p);

    // for linked list implementation. the next-highest priority InputContext.
    InputContext* next = nullptr;
    InputContext* prev = nullptr;
};

// Used to distribute input events based on a priority stack system. 
    // Higher priority = recieve input first. Same priority? most recently pushed = recieve input first.
// Window's input events always fire no matter the context, so doing input handling there is bad (consider trying to handle w either moving forward or typing in UI or etc.)
class InputStack {
public:
    constexpr static int GUI_INPUT_PRIORITY = 1000;

    // For the love of god make sure you call the variant of Connect() taking a specific InputContext
    static inline Event<InputContext, InputObject>& onInput = Event<InputContext, InputObject>::New();
    static InputStack& Get();

    // popping is done via destruction of the returned context object.
    // despite me calling it a stack, destruction does not have to be in any particular order relative to the pushing.
    // higher priority contexts recieve input first.
	std::unique_ptr<InputContext> PushContext(int priority = 0);

    ~InputStack() = default;
private:
    InputStack();

    // head of the doubly-linked list. the highest priority InputContext
    InputContext* head = nullptr;

    Connection windowInputConnection;
};
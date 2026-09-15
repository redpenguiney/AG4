#pragma once
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "GLM/vec2.hpp"
#include <unordered_set>
#include "event.hpp"
#include "cursor.hpp"
#include <optional>
#include "input.hpp"

class Window {
public:
    static inline unsigned initialWindowWidth = 512;
    static inline unsigned initialWindowHeight = 512;
    static Window& Get();

    static inline bool GLFW_INIT = false; // Indicates whether GLFW is currently initialized.

    unsigned int width = 0;
    unsigned int height = 0;
    const bool doubleBuf = true;
    const bool vsync = true; // value ignored if !doubleBuf

    Event<Window, InputObject>& onInput;
    Event<Window, InputObject>& onInputDown;
    Event<Window, InputObject>& onInputUp;

    // passes scroll delta x/y
    Event<Window, double, double>& onScroll;

    // fired after Update() finishes
    Event<Window>& postInputProccessing;

    // fired during Update() before postInputProccesing if the window was resized that frame.
    // first uvec2 is old window size, secon is new window size
    Event<Window, glm::uvec2, glm::uvec2>& onWindowResize;

    Window() = delete;
    Window(const Window&) = delete;

    ~Window();

    inline float Aspect() const { return float(width) / float(height); }
    inline glm::uvec2 Size() const { return { width, height }; }

    // Processes user input and fires PostInputProcessing
    void Update();

    // If VSync is enabled, will yield until the frame can be displayed.
    // If VSync is disabled, I have no idea what happens (TODO)
    void FlipBuffers();

    // Returns true if the user is trying to close the window or Close() was called.
    bool ShouldClose();

    // Doesn't immediately close it, but will make all subsequent calls to ShouldClose() return true (meaning the program will exit at end of this frame)
    void Close();


    void SetMouseLocked(bool locked);
    bool IsMouseLocked() const;

    // User input stuff.
    // index is key enums provided by GLFW
    // TODO: these maps/vectors are def not thread safe, probably needs a rwlock
    std::unordered_set<InputType> PRESSED_KEYS;
    std::unordered_set<InputType> PRESS_BEGAN_KEYS;
    std::unordered_set<InputType> PRESS_ENDED_KEYS;
    std::unordered_set<InputObject> PRESS_BEGAN_EVENTS;
    std::unordered_set<InputObject> PRESS_ENDED_EVENTS;

    /*bool IsPressed(int key);
    bool IsPressBegan(int key);
    bool IsPressEnded(int key);*/

    /*bool LMB_DOWN = false;
    bool RMB_DOWN = false;
    bool LMB_BEGAN = false;
    bool RMB_BEGAN = false;
    bool LMB_ENDED = false;
    bool RMB_ENDED = false;*/

    //bool SHIFT_DOWN; // TODO
    //bool CTRL_DOWN; // TODO

    // TODO: RAW MOUSE option
    glm::dvec2 MOUSE_POS = { 0, 0 };
    glm::dvec2 MOUSE_DELTA = { 0, 0 }; // how much mouse has moved since last frame

    void UseCursor(const Cursor& cursor);

    // System provided cursors for your use. You can change these if you REALLY want to without any issues but that'd be dumb so don't.
    Cursor systemPointerCursor;
    Cursor systemTextEntryCursor;
    Cursor systemCrosshairCursor;
    Cursor systemSelectionCursor;
    Cursor systemHorizontalResizingCursor;
    Cursor systemVerticalResizingCursor;

private:
    bool windowIsVisible = false;

    Window(int widthh, int heightt);

    // may be null or refer to invalid memory, do not access through. Only here to avoid redundant GLFW calls to set the current cursor.
    const Cursor* currentCursor = nullptr;

    bool mouseLocked = false;

    GLFWwindow* glfwWindow;
    // callbacks are called when glfwPollEvents() is called (in Update())
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void ResizeCallback(GLFWwindow* window, int newWindowWidth, int newWindowHeight);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void ScrollCallback(GLFWwindow* window, double deltaScrollX, double deltaScrollY);
}; 
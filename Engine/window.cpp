#include "window.hpp"
#include "gl_error_handler.cpp"
#include "graphics_engine.hpp"

#include <cstdio>
#include <cstdlib>

void ErrorCallback(int errorCode, const char* errorMessage) {
    DebugLogError("GLFW error ", errorCode, ": ", errorMessage);
    Assert(false);
}

Window& Window::Get() {
    static Window w(initialWindowWidth, initialWindowHeight);
    return w;
}

Window::Window(int widthh, int heightt) :
    inputDown(Event<Window, InputObject>::New()),
    inputUp(Event<Window, InputObject>::New()),
    onScroll(Event<Window, double, double>::New()),
    postInputProccessing(Event<Window>::New()),
    onWindowResize(Event<Window, glm::uvec2, glm::uvec2>::New())
{
    width = widthh;
    height = heightt;
    mouseLocked = false;
    auto initSuccess = glfwInit();
    if (!initSuccess) {
        std::printf("Failure to initialize GLFW. Aborting.\n");
        abort();
    }
    GLFW_INIT = true;

    glfwWindowHint(GLFW_DOUBLEBUFFER, doubleBuf ? GL_TRUE : GL_FALSE); // disbable double buffering; TODO THIS SHOULD NOT BE NECCESSARY
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE); // Tell GLFW we are going to be running opengl in debug mode, which lets us use GL_DEBUG_OUTPUT to get error messages easily
    
    // request core profile in latest opengl version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // make window initially invisible so that user won't see pure white/black/etc. until first frame is drawn
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    glfwWindow = glfwCreateWindow(width, height, "AG4", nullptr, nullptr);
    if (!glfwWindow) {
        glfwTerminate();
        std::printf("Failure to create GLFW window. Aborting.\n");
        abort();
    }

    glfwMakeContextCurrent(glfwWindow);
    glfwSetKeyCallback(glfwWindow, KeyCallback); // TODO: for text input we want character callback, not key callback
    glfwSetScrollCallback(glfwWindow, ScrollCallback);
    glfwSetMouseButtonCallback(glfwWindow, MouseButtonCallback);
    glfwSetFramebufferSizeCallback(glfwWindow, ResizeCallback);
    glfwSetErrorCallback(ErrorCallback);

    glfwSwapInterval(vsync ? 1 : 0); // do vsync

    GLenum glewSuccess = glewInit();
    if (glewSuccess != GLEW_OK) {
        glfwTerminate();
        std::printf("Failure to initalize GLEW (error %s). Aborting.\n", glewGetErrorString(glewSuccess));
        abort();
    }

    DebugLogInfo("Using OpenGL version ", glGetString(GL_VERSION));

    // TODO: remove
    glfwSetWindowPos(glfwWindow, 540, 180);

    // initialize mouse position
    glfwGetCursorPos(glfwWindow, &MOUSE_POS.x, &MOUSE_POS.y);
    // std::printf("Init mouse at %f %f\n", MOUSE_POS.x, MOUSE_POS.y);

    // tell glfw we care about capslock and numpad
    glfwSetInputMode(glfwWindow, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

    // grab system-provided cursors
    systemPointerCursor = Cursor(GLFW_ARROW_CURSOR);
    systemTextEntryCursor = Cursor(GLFW_IBEAM_CURSOR);
    systemCrosshairCursor = Cursor(GLFW_CROSSHAIR_CURSOR);
    systemSelectionCursor = Cursor(GLFW_HAND_CURSOR);
    systemHorizontalResizingCursor = Cursor(GLFW_HRESIZE_CURSOR);
    systemVerticalResizingCursor = Cursor(GLFW_VRESIZE_CURSOR);

    DebugLogInfo("Window creation successful.");

    // See gl_error_handler, just prints opengl errors to console automatically
    // todo: disable on release builds for performance
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // make sure it actually prints out the errors on the thread that created the error so you actually get a useful stack trace
    glDebugMessageCallback(MessageCallback, 0);


};

Window::~Window() {

    glfwTerminate();
    GLFW_INIT = false;
}

bool Window::IsMouseLocked() const {
    return mouseLocked;
}

//bool Window::IsPressed(int key) {
//    return PRESSED_KEYS.contains(key);
//}
//bool Window::IsPressBegan(int key) {
//    return PRESS_BEGAN_KEYS.contains(key);
//}
//bool Window::IsPressEnded(int key) {
//    return PRESS_ENDED_KEYS.contains(key);
//}

void Window::Update() {
    PRESS_BEGAN_KEYS.clear();
    PRESS_ENDED_KEYS.clear();
    PRESS_BEGAN_EVENTS.clear();
    PRESS_ENDED_EVENTS.clear();
    /*LMB_BEGAN = false;
    LMB_ENDED = false;
    RMB_BEGAN = false;
    RMB_ENDED = false;*/

    // set cursor pos
    glm::dvec2 pos;
    glfwGetCursorPos(glfwWindow, &pos.x, &pos.y);
    // std::printf("Old mouse pos was %f %f\n", MOUSE_POS.x, MOUSE_POS.y);
    // std::printf("Now it at %f %f\n", pos.x, pos.y);
    MOUSE_DELTA = pos - MOUSE_POS;
    MOUSE_POS = pos;

    // fire callbacks/input events
    glfwPollEvents();



    postInputProccessing.Fire(this);
}

void Window::FlipBuffers() {
    //glfwSwapInterval(1);
    if (!windowIsVisible) {
        glfwShowWindow(glfwWindow);
        windowIsVisible = true;
    }
    if (vsync)
        glfwSwapBuffers(glfwWindow);
    else {
        glFinish();
    }
}

// returns true if the user is trying to close the application, or if Window::Close() was explicitly called (like by a quit game button)
bool Window::ShouldClose() {
    return glfwWindowShouldClose(glfwWindow);
}

void Window::Close() {
    glfwSetWindowShouldClose(glfwWindow, true);
}

// TODO: when disabling mouse lock MOUSE_DELTA has a weird value
void Window::SetMouseLocked(bool locked) {
    mouseLocked = locked;
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, (locked) ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    // prevent mouse delta from going crazy when this is disabled
    glfwGetCursorPos(glfwWindow, &MOUSE_POS.x, &MOUSE_POS.y);
}

InputType glfwKeyToInputType(int key) {
    switch (key) {
    case GLFW_KEY_BACKSPACE:
        return InputType::Backspace;
    case GLFW_KEY_A:
        return InputType::A;
    case GLFW_KEY_B:
        return InputType::B;
    case GLFW_KEY_C:
        return InputType::C;
    case GLFW_KEY_D:
        return InputType::D;
    case GLFW_KEY_E:
        return InputType::E;
    case GLFW_KEY_F:
        return InputType::F;
    case GLFW_KEY_G:
        return InputType::G;
    case GLFW_KEY_H:
        return InputType::H;
    case GLFW_KEY_I:
        return InputType::I;
    case GLFW_KEY_J:
        return InputType::J;
    case GLFW_KEY_K:
        return InputType::K;
    case GLFW_KEY_L:
        return InputType::L;
    case GLFW_KEY_M:
        return InputType::M;
    case GLFW_KEY_N:
        return InputType::N;
    case GLFW_KEY_O:
        return InputType::O;
    case GLFW_KEY_P:
        return InputType::P;
    case GLFW_KEY_Q:
        return InputType::Q;
    case GLFW_KEY_R:
        return InputType::R;
    case GLFW_KEY_S:
        return InputType::S;
    case GLFW_KEY_T:
        return InputType::T;
    case GLFW_KEY_U:
        return InputType::U;
    case GLFW_KEY_V:
        return InputType::V;
    case GLFW_KEY_W:
        return InputType::W;
    case GLFW_KEY_X:
        return InputType::X;
    case GLFW_KEY_Y:
        return InputType::Y;
    case GLFW_KEY_Z:
        return InputType::Z;
    case GLFW_KEY_0:
        return InputType::Zero;
    case GLFW_KEY_1:
        return InputType::One;
    case GLFW_KEY_2:
        return InputType::Two;
    case GLFW_KEY_3:
        return InputType::Three;
    case GLFW_KEY_4:
        return InputType::Four;
    case GLFW_KEY_5:
        return InputType::Five;
    case GLFW_KEY_6:
        return InputType::Six;
    case GLFW_KEY_7:
        return InputType::Seven;
    case GLFW_KEY_8:
        return InputType::Eight;
    case GLFW_KEY_9:
        return InputType::Nine;
    case GLFW_KEY_GRAVE_ACCENT:
        return InputType::Grave;
    case GLFW_KEY_SPACE:
        return InputType::Space;
    case GLFW_KEY_ESCAPE:
        return InputType::Escape;
    case GLFW_KEY_TAB:
        return InputType::Tab;
    case GLFW_KEY_LEFT_ALT: // TODO: different alts bind to different enums???
        return InputType::Alt;
    case GLFW_KEY_RIGHT_ALT:
        return InputType::Alt;
    case GLFW_KEY_LEFT_BRACKET:
        return InputType::LBracket;
    case GLFW_KEY_RIGHT_BRACKET:
        return InputType::RBracket;
    case GLFW_KEY_UP:
        return InputType::UpArrow;
    case GLFW_KEY_DOWN:
        return InputType::DownArrow;
    case GLFW_KEY_LEFT:
        return InputType::LeftArrow;
    case GLFW_KEY_RIGHT:
        return InputType::RightArrow;
    case GLFW_KEY_LEFT_CONTROL:
        return InputType::Ctrl;
    case GLFW_KEY_LEFT_SUPER: // windows key, here to avoid an annoying unrecognized key msg 
        return InputType::Unknown;
    case GLFW_KEY_RIGHT_SHIFT:
    case GLFW_KEY_LEFT_SHIFT:
        return InputType::Shift;
    case GLFW_KEY_UNKNOWN:
        DebugLogError("Unrecognized key. Even GLFW doesn't know it.");
        return InputType::Unknown;
    default:
        DebugLogError("Unrecognized key ", key, " (GLFW recognizes it, but we don't.).");
        return InputType::Unknown;
    }
}

void Window::UseCursor(const Cursor& cursor)
{
    Assert(cursor.cursorHandle != nullptr);
    if (currentCursor == &cursor) return;
    currentCursor = &cursor;
    glfwSetCursor(glfwWindow, cursor.cursorHandle);
}

// GLFW calls these functions automatically when glfwPollEvents() is called.
void Window::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    InputObject input{
        .input = glfwKeyToInputType(key),
        .capitalized = (mods & GLFW_MOD_CAPS_LOCK) != (mods & GLFW_MOD_SHIFT),
        .altDown = bool(mods & GLFW_MOD_ALT),
        .ctrlDown = bool(mods & GLFW_MOD_CONTROL),
        .shiftDown = bool(mods & GLFW_MOD_SHIFT)
    };

    //DebugLogInfo("INPUT = ", input.input);

    if (action == GLFW_PRESS) {
        Window::Get().PRESS_BEGAN_KEYS.insert(input.input);
        Window::Get().PRESSED_KEYS.insert(input.input);
        Window::Get().inputDown.Fire(&Window::Get(), input);
        Window::Get().PRESS_BEGAN_EVENTS.insert(input);
    }
    else if (action == GLFW_RELEASE) {
        Window::Get().PRESS_ENDED_KEYS.insert(input.input);
        Window::Get().PRESSED_KEYS.erase(input.input);
        Window::Get().inputUp.Fire(&Window::Get(), input);
        Window::Get().PRESS_ENDED_EVENTS.insert(input);
    }
}

void Window::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {

    InputObject input{
        .capitalized = (mods & GLFW_MOD_CAPS_LOCK) != (mods & GLFW_MOD_SHIFT),
        .altDown = bool(mods & GLFW_MOD_ALT),
        .ctrlDown = bool(mods & GLFW_MOD_CONTROL),
        .shiftDown = bool(mods & GLFW_MOD_SHIFT)
    };

    if (button > GLFW_MOUSE_BUTTON_LAST) {
        DebugLogError("Goofy mouse button value of ", button);
    }
    else {
        input.input = InputType(static_cast<int>(InputType::LMB) + button);

        if (action == GLFW_RELEASE) {
            Window::Get().PRESS_ENDED_KEYS.insert(input.input);
            Window::Get().PRESSED_KEYS.erase(input.input);
            Window::Get().inputUp.Fire(&Window::Get(), input);
            Window::Get().PRESS_ENDED_EVENTS.insert(input);
        }
        else if (action == GLFW_PRESS) {
            Window::Get().PRESS_BEGAN_KEYS.insert(input.input);
            Window::Get().PRESSED_KEYS.insert(input.input);
            Window::Get().inputDown.Fire(&Window::Get(), input);
            Window::Get().PRESS_BEGAN_EVENTS.insert(input);
        }
    }



}

void Window::ScrollCallback(GLFWwindow* window, double deltaScrollX, double deltaScrollY)
{
    bool shift =Window::Get().PRESSED_KEYS.contains(InputType::Shift);
    bool alt =Window::Get().PRESSED_KEYS.contains(InputType::Alt);
    bool ctrl =Window::Get().PRESSED_KEYS.contains(InputType::Ctrl);

    auto input = InputObject{
        .input = InputType::Scroll, .direction = {deltaScrollX, deltaScrollY}, .capitalized = false, .altDown = alt, .ctrlDown = ctrl, .shiftDown = shift
    };

   Window::Get().onScroll.Fire(&Window::Get(), deltaScrollX, deltaScrollY);
   Window::Get().inputDown.Fire(&Window::Get(), input);
   Window::Get().PRESS_BEGAN_KEYS.insert(input.input);
   Window::Get().PRESS_ENDED_KEYS.insert(input.input);
   Window::Get().PRESS_BEGAN_EVENTS.insert(input);
   Window::Get().PRESS_ENDED_EVENTS.insert(input);
}

void Window::ResizeCallback(GLFWwindow* window, int newWindowWidth, int newWindowHeight) { // called on window resize
    // Tell OpenGL to draw to the whole screen (TODO: Window class should not be concerned with the graphics library)
    glViewport(0, 0, newWindowWidth, newWindowHeight);

    if (newWindowWidth != 0 && newWindowHeight != 0) {
        glm::uvec2 oldWidth(Window::Get().width, Window::Get().height);
       Window::Get().width = newWindowWidth;
       Window::Get().height = newWindowHeight;

        Assert(newWindowWidth != 0);

       Window::Get().onWindowResize.Fire(&Window::Get(), oldWidth, glm::uvec2(newWindowWidth, newWindowHeight));
    }

}

std::optional<std::string> InputToString(InputObject input) {
    if (input.capitalized) {
        switch (input.input) {
        case InputType::A:
            return "A";
        case InputType::B:
            return "B";
        case InputType::C:
            return "C";
        case InputType::D:
            return "D";
        case InputType::E:
            return "E";
        case InputType::F:
            return "F";
        case InputType::G:
            return "G";
        case InputType::H:
            return "H";
        case InputType::I:
            return "I";
        case InputType::J:
            return "J";
        case InputType::K:
            return "K";
        case InputType::L:
            return "L";
        case InputType::M:
            return "M";
        case InputType::N:
            return "N";
        case InputType::O:
            return "O";
        case InputType::P:
            return "P";
        case InputType::Q:
            return "Q";
        case InputType::R:
            return "R";
        case InputType::S:
            return "S";
        case InputType::T:
            return "T";
        case InputType::U:
            return "U";
        case InputType::V:
            return "V";
        case InputType::W:
            return "W";
        case InputType::X:
            return "X";
        case InputType::Y:
            return "Y";
        case InputType::Z:
            return "Z";
        }
    }
    else {
        switch (input.input) {
        case InputType::A:
            return "a";
        case InputType::B:
            return "b";
        case InputType::C:
            return "c";
        case InputType::D:
            return "d";
        case InputType::E:
            return "e";
        case InputType::F:
            return "f";
        case InputType::G:
            return "g";
        case InputType::H:
            return "h";
        case InputType::I:
            return "i";
        case InputType::J:
            return "j";
        case InputType::K:
            return "k";
        case InputType::L:
            return "l";
        case InputType::M:
            return "m";
        case InputType::N:
            return "n";
        case InputType::O:
            return "o";
        case InputType::P:
            return "p";
        case InputType::Q:
            return "q";
        case InputType::R:
            return "r";
        case InputType::S:
            return "s";
        case InputType::T:
            return "t";
        case InputType::U:
            return "u";
        case InputType::V:
            return "v";
        case InputType::W:
            return "w";
        case InputType::X:
            return "x";
        case InputType::Y:
            return "y";
        case InputType::Z:
            return "z";
        }
    }

    switch (input.input) {
    case InputType::Space:
        return " ";
    case InputType::Tab:
        return "\t";
    case InputType::Zero:
        return "0";
    case InputType::One:
        return "1";
    case InputType::Two:
        return "2";
    case InputType::Three:
        return "3";
    case InputType::Four:
        return "4";
    case InputType::Five:
        return "5";
    case InputType::Six:
        return "6";
    case InputType::Seven:
        return "7";
    case InputType::Eight:
        return "8";
    case InputType::Nine:
        return "9";
    default:
        return std::nullopt;
    }

    Assert(false);
    //std::unreachable();
}

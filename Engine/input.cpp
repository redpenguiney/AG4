#include "input.hpp"
#include <window.hpp>

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
    case InputType::Apostrophe:
        return input.shiftDown ? "\"" : "\'";
    case InputType::Semicolon:
        return input.shiftDown ? ":" : ";";
    case InputType::Comma:
        return input.shiftDown ? "<" : ",";
    case InputType::Period:
        return input.shiftDown ? ">" : ".";
    case InputType::Enter:
        return "\n";
    case InputType::Zero:
        return input.shiftDown ? ")" : "0";
    case InputType::One:
        return input.shiftDown ? "!" : "1";
    case InputType::Two:
        return input.shiftDown ? "@" : "2";
    case InputType::Three:
        return input.shiftDown ? "#" : "3";
    case InputType::Four:
        return input.shiftDown ? "$" : "4";
    case InputType::Five:
        return input.shiftDown ? "%" : "5";
    case InputType::Six:
        return input.shiftDown ? "^" : "6";
    case InputType::Seven:
        return input.shiftDown ? "&" : "7";
    case InputType::Eight:
        return input.shiftDown ? "*" : "8";
    case InputType::Nine:
        return input.shiftDown ? "(" : "9";
    case InputType::Minus:
        return input.shiftDown ? "_" : "-";
    case InputType::Equals:
        return input.shiftDown ? "+" : "=";
    case InputType::ForwardSlash:
        return input.shiftDown ? "?" : "/";
    case InputType::BackSlash:
        return input.shiftDown ? "|" : "\\";
    case InputType::Grave:
        return input.shiftDown ? "~" : "`";
    case InputType::LBracket:
        return input.shiftDown ? "{" : "[";
    case InputType::RBracket:
        return input.shiftDown ? "}" : "]";
    default:
        return std::nullopt;
    }

    Assert(false);
    //std::unreachable();
}

InputStack& InputStack::Get() {
    static InputStack stack;
    return stack;
}

std::unique_ptr<InputContext> InputStack::PushContext(int priority) {

    InputContext* c = new InputContext(priority);
    if (!head) {
        head = c;
    }
    else {
        InputContext** current = &head;
        while (true) {
            if ((**current).priority <= priority) {
                c->next = *current;
                c->prev = (**current).prev;
                *current = c;
                break;
            }
            else if ((**current).next == nullptr) {
                c->prev = *current;
                (**current).next = c;
            }
            else {
                current = &(**current).next;
            }
        }
    }
    return std::unique_ptr<InputContext>(c);
}

InputStack::InputStack() {
    windowInputConnection = Window::Get().onInput.Connect([this](Window*, InputObject input) {
        if (head) {
            onInput.Fire(head, input);
        }
        });
}

InputContext::~InputContext() {
    if (next) {
        next->prev = prev;
    }

    if (prev) {
        prev->next = next;
    }
}

void InputContext::Fallthrough(InputObject input) {
    if (next) {
        InputStack::onInput.Fire(next, input);
    }
}

InputContext::InputContext(int p): priority(p) {
}

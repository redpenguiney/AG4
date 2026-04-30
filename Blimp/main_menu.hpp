#pragma once
#include <memory>
#include "event.hpp"
class GuiElement;

inline std::shared_ptr<GuiElement> menuContainer;
inline std::vector<Connection> menuEventConnections;

void MakeMainMenu();
#pragma once
#include "debug_prefabs.hpp"
#include "gameobject.hpp"
#include <window.hpp>
#include <mainloop.hpp>

struct HandlesState;
void TransformHandles(Gameobject* target);

void ReportCollisions(Gameobject* a, Gameobject* b);
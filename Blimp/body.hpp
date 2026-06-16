#pragma once
#include "ik.hpp"

class Gameobject;

class Body {
public:
	Body();

private:
	std::vector<std::unique_ptr<Gameobject>> gameobjects;
	IKSkeletonInfo ik;
};
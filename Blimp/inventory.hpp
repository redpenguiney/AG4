#pragma once
#include <vector>
#include <memory>

class Item;

class Inventory {
public:
	std::vector<std::unique_ptr<Item>> contents;
};
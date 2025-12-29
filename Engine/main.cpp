#include "gameobject.hpp"
#include "mainloop.hpp"

int main() {
	DebugLogInfo("Main function reached successfully.");

	std::vector<std::shared_ptr<Gameobject>> objects;
	for (unsigned i = 0; i < 16384; i++) {
		Gameobject* gameObj = Gameobject::New();
		//delete gameObj;
		std::shared_ptr<Gameobject> unique(gameObj);
		objects.push_back(unique);
	}

	objects.clear();

	Mainloop::Get().Run();

	DebugLogInfo("Main function body executed successfully.");
}
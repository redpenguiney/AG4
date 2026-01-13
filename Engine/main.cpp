#include "gameobject.hpp"
#include "mainloop.hpp"
#include "mesh_provider.hpp"

int main() {
	DebugLogInfo("Main function reached successfully.");
	auto mparams = MeshCreateParams::Default();
	mparams.vertices = { 0.0f, 1.0f };
	auto squareMesh = Mesh::New();

	GameobjectCreateParams p;
	p.mesh = squareMesh;

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
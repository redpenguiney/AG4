#include "gameobject.hpp"
#include "mainloop.hpp"
#include "mesh_provider.hpp"
#include "mesh.hpp"

int main() {
	DebugLogInfo("Main function reached successfully.");
	auto mparams = MeshCreateParams::Default();
	mparams.LoadObj("../models/");

	auto squareMesh = Mesh::New(std::move(mparams));

	GameobjectCreateParams p;
	p.mesh = squareMesh;

	std::vector<std::shared_ptr<Gameobject>> objects;
	for (unsigned i = 0; i < 16384; i++) {
		Gameobject* gameObj = Gameobject::New(p);
		//delete gameObj;
		std::shared_ptr<Gameobject> unique(gameObj);
		objects.push_back(unique);
	}

	objects.clear();

	Mainloop::Get().Run();

	DebugLogInfo("Main function body executed successfully.");
}
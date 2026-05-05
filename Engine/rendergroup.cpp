#include "rendergroup.hpp"
#include "gameobject.hpp"
#include "mesh.hpp"
#include "rendergraph_node.hpp"
#include "static_meshpool.hpp"

void RenderGroup::RemoveGameobject(Gameobject& obj) {
	Assert(!commands.empty());
	meshpool->RemoveInstance(obj.drawInstanceIndex);

	DebugLogInfo("Removing gameobject from ", this, " with ", commands.size());

	// TODO: data structure to accelerate this
	for (unsigned i = 0; i < commands.size(); i++) {
		if (commands[i].baseInstance <= obj.drawInstanceIndex && commands[i].baseInstance + commands[i].instanceCount > obj.drawInstanceIndex) {
			
			if (commands[i].baseInstance + commands[i].instanceCount - 1 == obj.drawInstanceIndex) {
				if (commands[i].instanceCount == 1) {
					commands[i] = commands.back();
					commands.pop_back();
				}
				else {
					commands[i].instanceCount--;
				}
			}
			else if (commands[i].baseInstance == obj.drawInstanceIndex) {
				if (commands[i].instanceCount == 1) {
					commands[i] = commands.back();
					commands.pop_back();
				}
				else {
					commands[i].baseInstance++;
					commands[i].instanceCount--;
				}
			}
			else {
				commands.emplace_back(commands[i]);
				commands.back().baseInstance = obj.drawInstanceIndex + 1;
				commands.back().instanceCount = commands[i].baseInstance + commands[i].instanceCount - commands.back().baseInstance;
				commands[i].instanceCount = obj.drawInstanceIndex - commands[i].baseInstance;
			}

			break;
		}
	}

	if (commands.empty()) {
		DebugLogInfo("Calling rendergroup destructor bc last object removed. (", drawPasses.size(), ")");

		for (unsigned i = 0; i < renderGroupsByMeshpool[meshpool.get()].size(); i++) {
			if (renderGroupsByMeshpool[meshpool.get()][i].get() == this) {

				auto p = meshpool.get();

				renderGroupsByMeshpool[p][i] = renderGroupsByMeshpool[p].back(); // object is destructed after this line
				renderGroupsByMeshpool[p].pop_back(); 

				if (renderGroupsByMeshpool[p].empty()) renderGroupsByMeshpool.erase(p);

				return;
			}
		}

		Assert(false);
	}

}

RenderGroup::~RenderGroup() {

	DebugLogInfo("Destroying rendergroup ", this);

	Assert(commands.empty()); //TODO: ???
	for (auto& pass : drawPasses) {
		for (unsigned groupI = 0; groupI < pass->drawnObjects.size(); groupI++) {
			if (pass->drawnObjects[groupI] == this) {
				pass->drawnObjects[groupI] = pass->drawnObjects.back();
				pass->drawnObjects.pop_back();
				
				//DebugLogInfo("Removed drawnobjects from pass ", pass->name, " ", pass->drawnObjects.size(), " remain.");
				//for (auto& obj : pass->drawnObjects) {
					//DebugLogInfo("\tGroup with ", obj->commands.size(), "");
				//}
				goto removed; // only breaks inner loop
			}
		}
		Assert(false); // if we didn't remove something then drawPasses was wrong.
		removed:;

		if (pass->drawnObjects.empty()) {
			DebugLogInfo("Pass ", pass->name, " is empty now.");

		}
	}

	//for (auto& p : drawPasses) {
	//	Assert(drawPassNumUsers.contains(p.get()));
	//	drawPassNumUsers[p.get()]--;
	//	if (drawPassNumUsers[p.get()] == 0) {
	//		drawPassNumUsers.erase(p.get());
	//		DebugLogInfo("Erasing pass.");
	//		GraphicsEngine::Get().renderGraph->RemovePass(p);
	//	}
	//}
}

void RenderGroup::AddGameobject(Gameobject& obj, const GameobjectCreateParams& params) {
	obj.drawInstanceIndex = meshpool->AddInstance();
	obj.renderGroup = this;
	obj.meshpool = meshpool.get();

	IndirectDrawCommand command;
	command.baseVertex = params.mesh->baseVertex;
	command.count = params.mesh->numIndices;
	command.firstIndex = params.mesh->firstIndex;
	command.baseInstance = obj.drawInstanceIndex;
	command.instanceCount = 1;
	AddDrawCommand(command);
}

RenderGroup::RenderGroup(std::vector<std::shared_ptr<DrawPass>> renderpasses, std::shared_ptr<Meshpool> meshpool, GLenum primitiveType) : primitiveType(primitiveType), drawPasses(renderpasses), meshpool(meshpool) {
	Assert(!renderpasses.empty());
	Assert(meshpool != nullptr);

	for (auto& pass : renderpasses) {
		pass->drawnObjects.push_back(this);
		if (!drawPassNumUsers.contains(pass.get())) {
			drawPassNumUsers[pass.get()] = 0;
			DebugLogInfo("New group ", this, " contains novel pass ", pass->name);
			GraphicsEngine::Get().renderGraph->AddPass(pass);
		}
		drawPassNumUsers[pass.get()]++;
	}
}

void RenderGroup::AddDrawCommand(IndirectDrawCommand cmd) {
	// TODO: datastructure to accelerate this
	for (auto it = commands.rbegin(); it != commands.rend(); it++) {
		if (it->baseInstance + it->instanceCount == cmd.baseInstance) {
			if (it->firstIndex == cmd.firstIndex) {
				it->instanceCount += cmd.instanceCount;
			}
			else {
				commands.push_back(cmd);
			}
			return;
		}
	}
	commands.push_back(cmd);
}

void RenderGroup::FindRendergroupForGameobject(Gameobject& obj, const GameobjectCreateParams& params) {
	RenderGroup* group = nullptr;

	if (renderGroupsByMeshpool.contains(params.mesh->pool.get())) {
		for (auto& g : renderGroupsByMeshpool.at(params.mesh->pool.get())) {
			if (g->primitiveType == params.primitiveType && g->drawPasses == params.renderPasses) {
				group = g.get();
				break;
			}
		}
	}

	if (!group) {
		group = new RenderGroup(params.renderPasses, params.mesh->pool, params.primitiveType);
		DebugLogInfo("Made group ", group, " for ", params.renderPasses[0]->name);
		if (renderGroupsByMeshpool.contains(params.mesh->pool.get())) DebugLogInfo("Already had meshpool in map.");
		renderGroupsByMeshpool[params.mesh->pool.get()].push_back(std::shared_ptr<RenderGroup>(group));
	}
	else {
		DebugLogInfo("Already had group ", group, " for ", params.renderPasses[0]->name);
	}

	group->AddGameobject(obj, params);
}

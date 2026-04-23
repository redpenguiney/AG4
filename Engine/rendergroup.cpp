#include "rendergroup.hpp"
#include "gameobject.hpp"
#include "mesh.hpp"
#include "rendergraph_node.hpp"
#include "static_meshpool.hpp"

void RenderGroup::RemoveGameobject(Gameobject& obj) {
	meshpool->RemoveInstance(obj.drawInstanceIndex);

	// TODO: data structure to accelerate this
	for (unsigned i = 0; i < commands.size(); i++) {
		if (commands[i].baseInstance <= obj.drawInstanceIndex && commands[i].baseInstance + commands[i].instanceCount > obj.drawInstanceIndex) {
			
			if (commands[i].baseInstance + commands[i].instanceCount - 1 == obj.drawInstanceIndex) {
				if (commands[i].instanceCount == 1) {
					commands[i] = commands.back();
					commands.pop_back();
				}
				else {
					commands[i].baseInstance--;
				}
			}
			else if (commands[i].baseInstance == obj.drawInstanceIndex) {
				if (commands[i].instanceCount == 1) {
					commands[i] = commands.back();
					commands.pop_back();
				}
				else {
					commands[i].baseInstance++;
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
		for (auto& p : drawPasses) {
			drawPassNumUsers[p.get()]--;
			if (drawPassNumUsers[p.get()] == 0) {
				drawPassNumUsers.erase(p.get());
				GraphicsEngine::Get().renderGraph->RemovePass(p);
			}
		}

		for (unsigned i = 0; i < renderGroupsByMeshpool[meshpool.get()].size(); i++) {
			if (renderGroupsByMeshpool[meshpool.get()][i].get() == this) {
				renderGroupsByMeshpool[meshpool.get()][i] = renderGroupsByMeshpool[meshpool.get()].back();
				renderGroupsByMeshpool[meshpool.get()].pop_back();
				
				return; // OBJECT IS DESTRUCTED AFTER THIS POINT
			}
		}
	}

}

RenderGroup::~RenderGroup() {
	//Assert(commands.empty()); TODO: ???
	for (auto& pass : drawPasses) {
		for (unsigned groupI = 0; groupI < pass->drawnObjects.size(); groupI++) {
			if (pass->drawnObjects[groupI] == this) {
				pass->drawnObjects[groupI] = pass->drawnObjects.back();
				pass->drawnObjects.pop_back();
				break; // only breaks inner loop
			}
		}
	}
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
	for (auto& pass : renderpasses) {
		pass->drawnObjects.push_back(this);
		if (!drawPassNumUsers.contains(pass.get())) {
			drawPassNumUsers[pass.get()] = 0;
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
		for (auto& [_, vec] : renderGroupsByMeshpool) {
			for (auto& g : vec) {
				if (g->primitiveType == params.primitiveType && g->drawPasses == params.renderPasses) {
					group = g.get();
					break;
				}
			}
		}
	}

	if (!group) {
		group = new RenderGroup(params.renderPasses, params.mesh->pool, params.primitiveType);
		renderGroupsByMeshpool[params.mesh->pool.get()].push_back(std::shared_ptr<RenderGroup>(group));
	}

	group->AddGameobject(obj, params);
}

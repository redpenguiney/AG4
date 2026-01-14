#include "rendergroup.hpp"
#include "gameobject.hpp"
#include "mesh.hpp"
#include "rendergraph_node.hpp"
#include "static_meshpool.hpp"

void RenderGroup::RemoveGameobject(Gameobject& obj) {
	meshpool->RemoveInstance(obj.render.instanceIndex);

	// TODO: data structure to accelerate this
	for (unsigned i = 0; i < commands.size(); i++) {
		if (commands[i].baseInstance <= obj.render.instanceIndex && commands[i].baseInstance + commands[i].instanceCount > obj.render.instanceIndex) {
			
			if (commands[i].baseInstance + commands[i].instanceCount - 1 == obj.render.instanceIndex) {
				commands[i].instanceCount--;
			}
			else if (commands[i].baseInstance == obj.render.instanceIndex) {
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
				commands.back().baseInstance = obj.render.instanceIndex + 1;
				commands.back().instanceCount = commands[i].baseInstance + commands[i].instanceCount - commands.back().baseInstance;
				commands[i].instanceCount = obj.render.instanceIndex - commands[i].baseInstance;
			}

			break;
		}
	}
}

void RenderGroup::AddGameobject(Gameobject& obj, GameobjectCreateParams& params) {
	obj.render.instanceIndex = meshpool->AddInstance();
	obj.render.group = this;
	obj.render.pool = meshpool.get();

	IndirectDrawCommand command;
	command.baseVertex = params.mesh->baseVertex;
	command.count = params.mesh->numIndices;
	command.firstIndex = params.mesh->firstIndex;
	command.baseInstance = obj.render.instanceIndex;
	command.instanceCount = 1;
	AddDrawCommand(command);
}

RenderGroup::RenderGroup(std::vector<std::shared_ptr<RenderPass>> renderpasses, std::shared_ptr<Meshpool> meshpool) : enable_shared_from_this(), renderPasses(renderpasses), meshpool(meshpool) {
	for (auto& pass : renderpasses) {
		pass->drawnObjects.push_back(this);
	}
	renderGroupsByMeshpool[meshpool.get()].push_back(shared_from_this());
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
}

void RenderGroup::FindRendergroupForGameobject(Gameobject& obj, GameobjectCreateParams& params) {
	RenderGroup* group;

	if (renderGroupsByMeshpool.contains(params.mesh->pool)) {
		for (auto& [_, vec] : renderGroupsByMeshpool) {
			for (auto& g : vec) {
				if (g->renderPasses == params.renderPasses) {
					group = g.get();
					break;
				}
			}
		}
	}

	if (!group) {
		group = std::shared_ptr<RenderGroup>(new RenderGroup(params.renderPasses, params.mesh->pool)).get();
	}

	group->AddGameobject(obj, params);
}

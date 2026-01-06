#include "rendergroup.hpp"
#include "gameobject.hpp"
#include "mesh.hpp"
#include "rendergraph_node.hpp"
#include "static_meshpool.hpp"

void RenderGroup::RemoveGameobject(Gameobject& obj) {
	meshpool->RemoveInstance(obj.render.instanceIndex);
}

void RenderGroup::AddGameobject(Gameobject& obj) {
	obj.render.instanceIndex = meshpool->AddInstance();
	obj.render.group = this;
	obj.render.pool = meshpool.get();
}

RenderGroup::RenderGroup(std::vector<std::shared_ptr<RenderPass>> renderpasses, std::shared_ptr<Meshpool> meshpool) : enable_shared_from_this(), renderPasses(renderpasses), meshpool(meshpool) {
	for (auto& pass : renderpasses) {
		pass->drawnObjects.push_back(shared_from_this());
	}
	renderGroupsByMeshpool[meshpool.get()].push_back(shared_from_this());
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
		group = std::make_shared<RenderGroup>(params.renderPasses, params.mesh->pool).get();
	}

	group->AddGameobject(obj);
}

#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include "indirect_draw_command.hpp"

class RenderPass;
class Meshpool;
class Gameobject;
struct GameobjectCreateParams;

class RenderGroup {
public:
	const GLenum primitiveType = GL_TRIANGLES;

	static void FindRendergroupForGameobject(Gameobject& obj, GameobjectCreateParams& params);

	const std::vector<std::shared_ptr<RenderPass>> renderPasses;

	// May remove the RenderGroup from renderGroupsByMeshpool if removing last gameobject, potentially causing group destruction if no other shared_ptr references exist.
	void RemoveGameobject(Gameobject& obj);

	~RenderGroup() = default;

private:
	void AddGameobject(Gameobject& obj, GameobjectCreateParams& params);

	RenderGroup(std::vector<std::shared_ptr<RenderPass>> renderPasses, std::shared_ptr<Meshpool> meshpool);

	std::shared_ptr<Meshpool> meshpool = nullptr;

	static inline std::unordered_map<Meshpool*, std::vector<std::shared_ptr<RenderGroup>>> renderGroupsByMeshpool;

	void AddDrawCommand(IndirectDrawCommand cmd);

	std::vector<IndirectDrawCommand> commands;

	friend class RenderGraph;
};
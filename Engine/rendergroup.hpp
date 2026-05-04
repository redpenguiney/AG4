#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include "indirect_draw_command.hpp"

class DrawPass;
class ProcessedDrawPass;
class Meshpool;
class Gameobject;
struct GameobjectCreateParams;

class RenderGroup {
public:
	const GLenum primitiveType;

	static void FindRendergroupForGameobject(Gameobject& obj, const GameobjectCreateParams& params);

	const std::vector<std::shared_ptr<DrawPass>> drawPasses;

	// May remove the RenderGroup from renderGroupsByMeshpool if removing last gameobject, potentially causing group destruction if no other shared_ptr references exist.
	void RemoveGameobject(Gameobject& obj);

	~RenderGroup();

private:
	void AddGameobject(Gameobject& obj, const GameobjectCreateParams& params);

	RenderGroup(std::vector<std::shared_ptr<DrawPass>> drawPasses, std::shared_ptr<Meshpool> meshpool, GLenum primitiveType);
	RenderGroup(const RenderGroup&&) = delete;

	std::shared_ptr<Meshpool> meshpool = nullptr;

	static inline std::unordered_map<Meshpool*, std::vector<std::shared_ptr<RenderGroup>>> renderGroupsByMeshpool;

	void AddDrawCommand(IndirectDrawCommand cmd);

	std::vector<IndirectDrawCommand> commands;

	// users are RenderGroups, not Gameobjects.
	static inline std::unordered_map<DrawPass*, CheckedUint> drawPassNumUsers;
	//std::unordered_map<std::string, DrawPass*> drawPassesInUse;

	friend class RenderGraph;
	friend class GraphicsEngine;
};
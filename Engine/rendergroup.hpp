#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
class RenderPass;
class Meshpool;
class Gameobject;
class GameobjectCreateParams;

class RenderGroup: std::enable_shared_from_this<RenderGroup> {
public:
	static void FindRendergroupForGameobject(Gameobject& obj, GameobjectCreateParams& params);

	const std::vector<std::shared_ptr<RenderPass>> renderPasses;

	void RemoveGameobject(Gameobject& obj);

private:
	void AddGameobject(Gameobject& obj);

	RenderGroup(std::vector<std::shared_ptr<RenderPass>> renderPasses, std::shared_ptr<Meshpool> meshpool);
	~RenderGroup();

	std::shared_ptr<Meshpool> meshpool;

	static std::unordered_map<Meshpool*, std::vector<std::shared_ptr<RenderGroup>>> renderGroupsByMeshpool;
};
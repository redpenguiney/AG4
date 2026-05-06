#include "rendergraph_node.hpp"
#include <log.hpp>

DrawPass::~DrawPass() {
	//DebugLogInfo("Draw pass destructor ran.");
}

std::shared_ptr<DrawPass> DrawPass::FromTemplate(const DrawPass& other)
{
	auto copy = new DrawPass(other);
	copy->drawnObjects.clear();
	return std::shared_ptr<DrawPass>(copy);
}

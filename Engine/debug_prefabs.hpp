#pragma once
#include <memory>

class Mesh;
class DrawPass;

std::shared_ptr<Mesh> GetCubeMesh();
std::shared_ptr<DrawPass> GetDebugWireframePass();
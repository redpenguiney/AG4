#pragma once
#include <utility>
#include <memory>
#include <functional>
#include <vector>
#include <optional>
#include "log.hpp"
#include <glm/vec3.hpp>
#include "texture_atlas.hpp"
#include <GL/glew.h>
#include "mesh_format.hpp"

class MeshCreateParams {
public:
	MeshVertexFormat meshVertexFormat = MeshVertexFormat::Default();

    std::vector<VertexScalar> vertices = {};
    std::vector<unsigned> indices = {};

    // Scales all vertices named vertexPos into the range [-0.5, 0.5], and sets Mesh::originalSize.
	// size should always be normalized for collisions/physics to work. Only set to false if you're making a weird mesh like the skybox or gui or something.
	// to actually change a rendercomponent's mesh's size, scale its transform component, using Mesh::originalSize if you want the mesh at its correct size.
	bool normalizeSize = true;
	bool generateNormals = false;
	bool generateTangents = false;
	// if true, the mesh cannot be changed dynamically, and will be stored in StaticMeshpool, which is not optimized for mesh removal.
	// Set to false for meshes you will repeatedly change and destroy like for procedural terrain.
	bool isStatic = true;

	// Equivalent to default constructor.
	static MeshCreateParams Default();
	static MeshCreateParams DefaultGui();

	// sets vertices and indices given path to a .obj file. Does nothing else.
	// meshVertexFormat should already be set to what you want when you call this.
	// Requires SpecialVertexAttributeNames to be used.
	void LoadObj(std::string path);

    MeshCreateParams() = default;
    MeshCreateParams(MeshCreateParams&&) noexcept = default;
    MeshCreateParams& operator=(MeshCreateParams&& other) noexcept = default;

private:

    MeshCreateParams(const MeshCreateParams&) = delete;
    
};

enum class HorizontalAlignMode {
	Left,
	Center,
	Right
};

enum class VerticalAlignMode {
	Top,
	Center,
	Bottom
};

struct TextMeshCreateParams {
	float leftMargin = -1000, rightMargin = 1000, topMargin = 0, bottomMargin = 0; // in pixels, 0 is the center of the ui text is being put on. top and bottom margin are only respected for top and bottom vertical alignment respectively.
	float lineHeightMultiplier = 1; // 1 is single spaced, 2 is double spaced, etc.
	HorizontalAlignMode horizontalAlignMode = HorizontalAlignMode::Center;
	VerticalAlignMode verticalAlignMode = VerticalAlignMode::Center;
	bool wrapText = true;
};
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

class Texture;

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

struct TextFormatting {
	// Text mesh will be strictly constrained by left and right margins if wrapping is enabled. 
	// If wrapping is not enabled, then left/right margins are only respected for left/right horizontal alignments respectively.
	// Top/bottom margins are only respected for top/bottom vertical alignments respectively.
	// Margins are in pixels.
	int leftMargin = -100000;
	int rightMargin = -100000;
	int topMargin = 0;
	int bottomMargin = 0;
	
	float lineHeightMultiplier = 1; // 1 is single spaced, 2 is double spaced, etc.

	HorizontalAlignMode horizontalAlignment = HorizontalAlignMode::Center;
	VerticalAlignMode verticalAlignment = VerticalAlignMode::Center;

	bool wrapping = true;
	int tabLength = 4; // in spaces
};

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
	// Written positions are in pixel space; you should probably NOT use normalizeSize with this.
	void LoadObj(std::string path);

	// sets vertices and indices given a font texture and 
	// meshVertexFormat should already be set to what you want when you call this. 
	// Only sets position, and requires SpecialVertexAttributeNames to be used for position.
	void LoadText(Texture& fontmap, std::string text, TextFormatting formatting);

    MeshCreateParams() = default;
    MeshCreateParams(MeshCreateParams&&) noexcept = default;
    MeshCreateParams& operator=(MeshCreateParams&& other) noexcept = default;

private:

    MeshCreateParams(const MeshCreateParams&) = delete;
    
};
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

	// size should always be normalized for collisions/physics to work. Only set to false if you're making a weird mesh like the skybox or gui or something.
	// to actually change a rendercomponent's mesh's size, scale its transform component, using Mesh::originalSize if you want the mesh at its correct size.
	bool normalizeSize = true;

	static MeshCreateParams Default();
	static MeshCreateParams DefaultGui();

    MeshCreateParams() = default;
    MeshCreateParams(TextMeshCreateParams&&) noexcept;
    MeshCreateParams& operator=(TextMeshCreateParams&& other) noexcept;

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

// Mesh provider that creates a mesh for text.
class TextMeshProvider: public MeshProvider {
public:
    TextMeshProvider(const MeshCreateParams& = MeshCreateParams::Default(), const std::shared_ptr<Material>& font = nullptr);

    std::pair < std::vector < VertexScalarType >, std::vector< unsigned int> > GetMesh() const override;

    std::string text = "Placeholder text.";

    // must be valid ptr to material with fontmap
    const std::shared_ptr<Material> font = nullptr;
	TextMeshCreateParams textParams;
};

// Mesh provider that uses dual contouring to generate a mesh based from a signed distance function representing a terrain surface.
// Created mesh samples points between p1 and p2 with the given resolution. 
// The distance function must be continuous, and return distance values between -1 and 1 (TODO CONFIRM)
// If a texture atlas is provided, it will use it for UVs and/or vertex colors if the format supports them.
// Will also generate normals, tangents, etc. if requested by the format.
// Mesh will be designed to be of dimensions size.
// Note that this provider may return an empty mesh (if the terrain area sampled is completely solid or completely hollow). TODO is that okay?
// TODO currently basically broken
class DualContouringMeshProvider: public MeshProvider {
public:
    DualContouringMeshProvider(const MeshCreateParams& = MeshCreateParams::Default());

    // defined in mesh_voxels.cpp
    std::pair < std::vector < VertexScalarType >, std::vector< unsigned int> > GetMesh() const override;

    glm::vec3 point1;
    glm::vec3 point2;
    float resolution;
    std::function<float(glm::vec3)> distanceFunction;
    std::optional<const TextureAtlas*> atlas = std::nullopt; // sorry its a pointer, should be a reference but it doesn't like optionals with references.
    bool fixVertexCenters = false; // if true will create minecraft-style blocky terrain
};

// Mesh provider that uses marching cubes based on a signed distance function.
class MarchingCubesMeshProvider : public MeshProvider {
public:
    MarchingCubesMeshProvider(const MeshCreateParams & = MeshCreateParams::Default());

    // defined in mesh_voxels.cpp
    std::pair < std::vector < float >, std::vector< unsigned int> > GetMesh() const override;

    glm::vec3 point1;
    glm::vec3 point2;
    float resolution;
    std::function<float(glm::vec3)> distanceFunction;
    std::optional<const TextureAtlas*> atlas = std::nullopt; // sorry its a pointer, should be a reference but it doesn't like optionals with references.
    bool fixVertexCenters = false; // if true will create minecraft-style blocky terrain
};
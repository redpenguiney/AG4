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

enum class VertexScalarType {
    f32,
    i32,
    u32
};

union VertexScalar {
    float f;
    unsigned u;
    int i;
};

class Material;

// Something a vertex has; color, position, normal, etc.
struct VertexAttribute {
    std::string name = "unnamed";

    // nComponents must == 16 and instanced must == true if this == true. All meshvertexformats should have exactly 1 attribute for which this value is true.
    bool writeModelMatrix = false;

    // the offset, in bytes, of the vertex attribute.
    // Calculated automatically; user modifications to this value are pointless and will be overwritten.
    unsigned offset;

    // the number of floats in each vertex attribute. (position would be 3 in a 3D situation, for example).
    // Must be greater than 0 and <= 4, or == 9 (for a 3x3 matrix) or == 16 (for 4x4 matrices). TODO support other matrices
    unsigned nComponents;

    // whether this vertex attribute is for every vertex, or for every renderComponent. (so you could either give each vertex a different color, or just store one color for each object)
    bool instanced;

    VertexScalarType type = VertexScalarType::f32;

    bool operator==(const VertexAttribute& other) const = default;
};

// Describes which vertex attributes a mesh has, which of them are instanced, and in what order they are in.
struct MeshVertexFormat {
    
    MeshVertexFormat(std::vector<VertexAttribute> attribs);
    MeshVertexFormat(const MeshVertexFormat&) = default;

    const std::vector<VertexAttribute>& GetAttributes() const;

    

    // returns combined size in bytes of each non-instanced vertex attribute for one vertex
    unsigned int GetNonInstancedVertexSize() const;

    // returns combined size in bytes of each instanced vertex attribute for one vertex
    unsigned int GetInstancedVertexSize() const;

    // Returns a simple mesh vertex format that should work for normal people doing normal things in 3D.
    // nBones should equal 0 if you don't want animations, otherwise MUST be multiple of 4.
    // noninstanced (XYZ, TextureXY, NormalXYZ, TangentXYZ, RGBA if !instanceColor, TextureZ if !instanceTextureZ). TODO make actually accurate
    // instanced: model matrix, normal matrix, rgba if instanced, textureZ if instanced
    static MeshVertexFormat Default(unsigned int nBones = 0, bool instancedColor = true, bool instancedTextureZ = true);

    // Returns a simple mesh vertex format that should work for normal people doing normal things with GUI. Just XYZ UV (plus instanced stuff).
    static MeshVertexFormat DefaultGui();

    // triplanar mapping is a shader technique that allows for texturing meshes that don't have UVs, which is useful for things like procedural terrain.
    // Has to be used with a buffer that corresponds vertex indices to textureZ/atlas data.
    // noninstanced: XYZ, NormalXYZ, RGBA if !instanceColor.
    // instanced:  model matrix, normal matrix, rgba if instanced
    static MeshVertexFormat DefaultTriplanarMapping(bool instancedColor = true);

private:
    std::vector<VertexAttribute> attributes;
    unsigned noninstancedVertexSize;
    unsigned instancedVertexSize;
};

struct MeshCreateParams {
	MeshVertexFormat meshVertexFormat = MeshVertexFormat::Default();

	// default value of textureZ for the mesh's vertices, if that is a noninstanced vertex attribute
	float textureZ = -1.0;

	// default value of transaprency/alpha for the mesh's vertices, if color is a noninstanced 4-component vertex attribute
	float opacity = 1.0;

	// meshpool will make room for this many instances/gameobjects using this mesh (if you go over it's fine, but performance may be affected)
	// the memory cost of this is ~64 * expectedCount bytes if you ask for space you don't nee
	// default of 1024 should be fine in almost all cases
	// NOTE: if you're constantly adding and removing unique meshes, they better all have same expectedCount or TODO memory issues i should probably address at somepoint
	unsigned int expectedCount = 1024;

	// size should always be normalized for collisions/physics to work. Only set to false if you're making a weird mesh like the skybox or gui or something.
	// to actually change a rendercomponent's mesh's size, scale its transform component, using Mesh::originalSize if you want the mesh at its correct size.
	bool normalizeSize = true;

	static MeshCreateParams Default();
	static MeshCreateParams DefaultGui();
};

// When you create a Mesh, it needs to get its vertices and indices from somewhere.
// When loading from files via assimp, you use Mesh::MultiFromFile().
	// This is because assimp decides basically everything about the meshes (yes, plural which is why assimp file loading doesn't use MeshProvider).
// For other stuff, you use Mesh::New() and provide a object that inherits from MeshProvider.
class MeshProvider {
public:
    MeshProvider(const MeshCreateParams&);
	MeshCreateParams meshParams = MeshCreateParams::Default();

	// returns the vertices and indices.
	virtual std::pair < std::vector < VertexScalarType > , std::vector< unsigned int> > GetMesh() const = 0;

	virtual ~MeshProvider() = default;
};

// Mesh provider that takes user-specified vertices and indices.
// verts must be organized in accordance with the given meshVertexFormat.
class RawMeshProvider: public MeshProvider {
public:
	RawMeshProvider(const std::vector<VertexScalarType>& vertices = {}, const std::vector<unsigned int>& indices = {}, const MeshCreateParams& params = MeshCreateParams::Default());

    std::pair < std::vector < VertexScalarType >, std::vector< unsigned int> > GetMesh() const override;

	std::vector<VertexScalarType> vertices;
	std::vector<unsigned int> indices;
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
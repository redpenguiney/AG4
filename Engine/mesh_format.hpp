#pragma once
#include <vector>
#include <string>

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
    // The name used to identify this vertex attribute in shaders. TODO: actually use seperate vao for each shader to facillitate this
    // Certain names (enumerated in the namespace SpecialVertexAttributeNames) have special behaviour and should be used when possible.
    // MODEL_MATRIX and NORMAL_MATRIX are required and must be instanced with 16 and 9 floats respectively.
    // A format cannot have two attributes with the same name.
    std::string name = "unnamed";

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

namespace SpecialVertexAttributeNames {
    const inline std::string VERTEX_POSITION = "vertexPos";
    const inline std::string MODEL_MATRIX = "modelMatrix";
    const inline std::string NORMAL_MATRIX = "normalMatrix";
    const inline std::string VERTEX_NORMAL = "vertexNormal";
    const inline std::string VERTEX_TANGENT = "vertexTangent";
    const inline std::string VERTEX_UV = "textureXY";


};

// Describes which vertex attributes a mesh has, which of them are instanced, and in what order they are in.
// Attribute offsets are always correct.
struct MeshVertexFormat {

    MeshVertexFormat(std::vector<VertexAttribute> attribs);
    MeshVertexFormat(const MeshVertexFormat&) = default;

    const std::vector<VertexAttribute>& GetAttributes() const;

    unsigned ScalarsPerVertex() const;

    // returns combined size in bytes of each non-instanced vertex attribute for one vertex
    unsigned int GetNonInstancedVertexSize() const;

    // returns combined size in bytes of each instanced vertex attribute for one vertex
    unsigned int GetInstancedVertexSize() const;

    // Returns a simple mesh vertex format that should work for normal people doing normal things in 3D.
    // noninstanced: XYZ, TextureXY, NormalXYZ, TangentXYZ. 
    // instanced: model matrix, normal matrix, rgba, textureZ 
    static MeshVertexFormat Default();

    // Returns a simple mesh vertex format that should work for normal people doing normal things with GUI. 
    // noninstanced XYZ UV 
    // instanced model matrix, normal matrix, rgba, textureZ
    static MeshVertexFormat DefaultGui();

    // triplanar mapping is a shader technique that allows for texturing meshes that don't have UVs, which is useful for things like procedural terrain.
    // Has to be used with a buffer that corresponds vertex indices to textureZ/atlas data.
    // noninstanced: XYZ, NormalXYZ, RGBA if !instanceColor.
    // instanced:  model matrix, normal matrix, rgba if instanced
    static MeshVertexFormat DefaultTriplanarMapping(bool instancedColor = true);

    // Returns pointer to attribute with given name if it exists, or nullptr if it does not.
    // Pointer is obviously invalidated if anything happens to the format object or its attributes.
    VertexAttribute* GetAttribute(std::string name);

    bool operator==(const MeshVertexFormat& other) const = default;

private:
    std::vector<VertexAttribute> attributes;
    unsigned noninstancedVertexSize;
    unsigned instancedVertexSize;
};

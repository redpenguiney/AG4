#include "mesh_format.hpp"
#include "assert.hpp"
#include "GL/glew.h"
#include <unordered_set>


unsigned int MeshVertexFormat::GetInstancedVertexSize() const {
    return instancedVertexSize;
}

unsigned int MeshVertexFormat::GetNonInstancedVertexSize() const {
    return noninstancedVertexSize;
}

MeshVertexFormat::MeshVertexFormat(std::vector<VertexAttribute> attribs) : attributes(attribs) {

    unsigned numModelMatrices = 0;
    unsigned numNormalMatrices = 0;

    std::unordered_set<std::string> names;

    // calculate attribute offsets and sizes
    unsigned int noninstancedOffset = 0, instancedOffset = 0;

    for (auto& attrib : attributes) {
        Assert(!names.contains(attrib.name));
        names.insert(attrib.name);
        if (attrib.instanced) {
            if (attrib.name == SpecialVertexAttributeNames::MODEL_MATRIX) {
                numModelMatrices++;
                Assert(attrib.nComponents == 16);
            }
            if (attrib.name == SpecialVertexAttributeNames::NORMAL_MATRIX) {
                numNormalMatrices++;
            }
            attrib.offset = instancedOffset;
            instancedOffset += attrib.nComponents * sizeof(GLfloat);
        }
        else {
            Assert(attrib.name != SpecialVertexAttributeNames::MODEL_MATRIX && attrib.name != SpecialVertexAttributeNames::NORMAL_MATRIX);
            attrib.offset = noninstancedOffset;
            noninstancedOffset += attrib.nComponents * sizeof(GLfloat);
        }
    }
    noninstancedVertexSize = noninstancedOffset;
    instancedVertexSize = instancedOffset;

    Assert(numModelMatrices == 1 && numNormalMatrices == 1);
}

const std::vector<VertexAttribute>& MeshVertexFormat::GetAttributes() const {
    return attributes;
}

unsigned MeshVertexFormat::ScalarsPerVertex() const {
    unsigned n = 0;
    for (auto& attr : attributes) {
        if (!attr.instanced) n += attr.nComponents;
    }
}


MeshVertexFormat MeshVertexFormat::Default() {
    std::vector<VertexAttribute> attributes;
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_POSITION, .nComponents = 3, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_UV, .nComponents = 2, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_NORMAL, .nComponents = 3, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_TANGENT, .nComponents = 3, .instanced = false });

    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::MODEL_MATRIX, .nComponents = 16, .instanced = true });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::NORMAL_MATRIX, .nComponents = 9, .instanced = true });
    attributes.push_back(VertexAttribute{ .name = "color", .nComponents = 4, .instanced = true });

    return MeshVertexFormat(attributes);
}

// noninstanced (XYZ, TextureXY).
// instanced: model matrix, normal matrix (so texture can be rotated w/o problems), rgba, textureZ
MeshVertexFormat MeshVertexFormat::DefaultGui() {
    std::vector<VertexAttribute> attributes;
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_POSITION, .nComponents = 3, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_UV, .nComponents = 2, .instanced = false });

    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::MODEL_MATRIX, .nComponents = 16, .instanced = true });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::NORMAL_MATRIX, .nComponents = 9, .instanced = true });
    attributes.push_back(VertexAttribute{ .name = "color", .nComponents = 4, .instanced = true });
    // TODO: arb1

    return MeshVertexFormat(attributes);
}

MeshVertexFormat MeshVertexFormat::DefaultTriplanarMapping(bool instancedColor)
{
    std::vector<VertexAttribute> attributes;
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_POSITION, .nComponents = 3, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_UV, .nComponents = 2, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_NORMAL, .nComponents = 3, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_TANGENT, .nComponents = 3, .instanced = false });

    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::MODEL_MATRIX, .nComponents = 16, .instanced = true });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::NORMAL_MATRIX, .nComponents = 9, .instanced = true });
    attributes.push_back(VertexAttribute{ .name = "color", .nComponents = 4, .instanced = true });
}

VertexAttribute* MeshVertexFormat::GetAttribute(std::string name) {
    for (auto& a : attributes) if (a.name == name) return &a;
    return nullptr;
}

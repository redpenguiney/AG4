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

MeshVertexFormat::MeshVertexFormat(std::vector<VertexAttribute> attribs, bool floatingOriginEnabled, unsigned maxBones) : attributes(attribs), floatingOriginEnabled(floatingOriginEnabled), boneCapacity(maxBones) {

    unsigned numModelMatrices = 0;
    unsigned numNormalMatrices = 0;

    std::unordered_set<std::string> names;

    // calculate attribute offsets and sizes
    unsigned int noninstancedOffset = 0, instancedOffset = 0;

    for (auto& attrib : attributes) {
        Assert(!names.contains(attrib.name));
        names.insert(attrib.name);
        Assert(attrib.nComponents > 0);
        if (attrib.instanced) {
            if (attrib.name == SpecialVertexAttributeNames::MODEL_MATRIX) {
                numModelMatrices++;
                Assert(attrib.nComponents == 16);
            }
            if (attrib.name == SpecialVertexAttributeNames::NORMAL_MATRIX) {
                numNormalMatrices++;
                Assert(attrib.nComponents == 9);
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

    Assert(noninstancedVertexSize > 0 && instancedVertexSize > 0);
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
    return n;
}


MeshVertexFormat MeshVertexFormat::Default(unsigned nBones) {
    std::vector<VertexAttribute> attributes;
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_POSITION, .nComponents = 3, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_UV, .nComponents = 2, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_NORMAL, .nComponents = 3, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_TANGENT, .nComponents = 3, .instanced = false });
    if (nBones > 0) {
        attributes.push_back(VertexAttribute{ .name = "boneWeights", .nComponents = 4, .instanced = false });
		attributes.push_back(VertexAttribute{ .name = "boneIDs", .nComponents = 4, .instanced = false, .type = VertexScalarType::i32 });
    }

    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::MODEL_MATRIX, .nComponents = 16, .instanced = true });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::NORMAL_MATRIX, .nComponents = 9, .instanced = true });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION, .nComponents = 1, .instanced = true });
    attributes.push_back(VertexAttribute{ .name = "color", .nComponents = 4, .instanced = true });

    return MeshVertexFormat(attributes);
}

// noninstanced (XYZ, TextureXY).
// instanced: model matrix, normal matrix (so texture can be rotated w/o problems), rgba, textureZ
MeshVertexFormat MeshVertexFormat::DefaultGui() {
    std::vector<VertexAttribute> attributes;
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_POSITION, .nComponents = 3, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::VERTEX_UV, .nComponents = 2, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION, .nComponents = 1, .instanced = true });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::MODEL_MATRIX, .nComponents = 16, .instanced = true });
    attributes.push_back(VertexAttribute{ .name = SpecialVertexAttributeNames::NORMAL_MATRIX, .nComponents = 9, .instanced = true });
    attributes.push_back(VertexAttribute{ .name = "color", .nComponents = 4, .instanced = true });
    // TODO: arb1

    return MeshVertexFormat(attributes, false);
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

    return MeshVertexFormat(attributes);

}

VertexAttribute* MeshVertexFormat::GetAttribute(std::string name) {
    for (auto& a : attributes) if (a.name == name) return &a;
    return nullptr;
}

const VertexAttribute* MeshVertexFormat::GetAttribute(std::string name) const
{
    for (const auto& a : attributes) if (a.name == name) return &a;
    return nullptr;
}

unsigned MeshVertexFormat::GetBoneCapacity() const {
	return boneCapacity;
}

bool MeshVertexFormat::IsFloatingOriginEnabled() const {
    return floatingOriginEnabled;
}

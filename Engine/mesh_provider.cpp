#include "mesh_provider.hpp"
#include "mesh.hpp"

MeshCreateParams MeshCreateParams::Default() {
    return MeshCreateParams();
}

MeshCreateParams MeshCreateParams::DefaultGui() {
    MeshCreateParams p;
    p.meshVertexFormat = MeshVertexFormat::DefaultGui();
    return p;
}

MeshCreateParams& MeshCreateParams::operator=(TextMeshCreateParams&& other) noexcept {
    return *this;
}

unsigned int MeshVertexFormat::GetInstancedVertexSize() const {
    return instancedVertexSize;
}

unsigned int MeshVertexFormat::GetNonInstancedVertexSize() const {
    return noninstancedVertexSize;
}

MeshVertexFormat::MeshVertexFormat(std::vector<VertexAttribute> attribs) : attributes(attribs) {

    unsigned numModelMatrices = 0;
    unsigned numNormalMatrices = 0;

    // calculate attribute offsets and sizes
    unsigned int noninstancedOffset = 0, instancedOffset = 0;
    
    for (auto& attrib : attributes) {
        if (attrib.instanced) {
            if (attrib.writeModelMatrix) {
                numModelMatrices++;
                Assert(attrib.nComponents == 16);
            }
            if (attrib.writeNormalMatrix) {
                numNormalMatrices++;
                Assert(!attrib.writeModelMatrix && attrib.nComponents == 9);
            }
            attrib.offset = instancedOffset;
            instancedOffset += attrib.nComponents * sizeof(GLfloat);
        }
        else {
            Assert(!attrib.writeModelMatrix);
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


MeshVertexFormat MeshVertexFormat::Default() {
    std::vector<VertexAttribute> attributes;
    attributes.push_back(VertexAttribute{ .name = "vertexPos", .nComponents = 3, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = "textureXY", .nComponents = 3, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = "vertexNormal", .nComponents = 3, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = "vertexPos", .nComponents = 3, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = "vertexPos", .nComponents = 3, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = "vertexPos", .nComponents = 3, .instanced = false });
    attributes.push_back(VertexAttribute{ .name = "vertexPos", .nComponents = 3, .instanced = false });

    attributes.push_back(VertexAttribute{ .name = "modelMatrix", .writeModelMatrix = true, .nComponents = 16, .instanced = true });
    attributes.push_back(VertexAttribute{ .name = "normalMatrix", .writeNormalMatrix = true, .nComponents = 9, .instanced = true });
    attributes.push_back(VertexAttribute{ .name = "vertexColor", .nComponents = 4, .instanced = true });

    return format;
}

// noninstanced (XYZ, TextureXY).
// instanced: model matrix, normal matrix (so texture can be rotated w/o problems), rgba, textureZ
MeshVertexFormat MeshVertexFormat::DefaultGui() {
    return MeshVertexFormat({
            .position = VertexAttribute {.offset = 0, .nComponents = 3, .instanced = false},
            .textureUV = VertexAttribute {.offset = sizeof(glm::vec3), .nComponents = 2, .instanced = false},
            .textureZ = VertexAttribute {.offset = (sizeof(glm::mat4x4) + sizeof(glm::mat3x3) + sizeof(glm::vec4)), .nComponents = 1, .instanced = true},
            .color = VertexAttribute {.offset = sizeof(glm::mat4x4) + sizeof(glm::mat3x3), .nComponents = 4, .instanced = true},
            .modelMatrix = VertexAttribute {.offset = 0, .nComponents = 16, .instanced = true},
            .normalMatrix = VertexAttribute {.offset = sizeof(glm::mat4x4), .nComponents = 9, .instanced = true},
            .normal = std::nullopt,
            .tangent = std::nullopt,
            .arbitrary1 = VertexAttribute {.offset = sizeof(glm::mat4x4) + sizeof(glm::mat3x3) + sizeof(glm::vec4) + sizeof(float), .nComponents = 1, .instanced = true}
        });
}

MeshVertexFormat MeshVertexFormat::DefaultTriplanarMapping(bool instancedColor)
{
    return MeshVertexFormat({
        .position = VertexAttribute {.nComponents = 3, .instanced = false},
        .color = VertexAttribute {.nComponents = 4, .instanced = instancedColor},
        .modelMatrix = VertexAttribute {.nComponents = 16, .instanced = true},
        .normalMatrix = VertexAttribute {.nComponents = 9, .instanced = true},
        .normal = VertexAttribute {.nComponents = 3, .instanced = false},
    });
}

MeshProvider::MeshProvider(const MeshCreateParams& p) : meshParams(p)
{
}



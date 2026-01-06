#include "mesh_provider.hpp"
#include "mesh.hpp"

MeshCreateParams MeshCreateParams::Default() {
    return MeshCreateParams();
}

MeshCreateParams MeshCreateParams::DefaultGui() {
    return MeshCreateParams({ .meshVertexFormat = MeshVertexFormat::DefaultGui() });
}

RawMeshProvider::RawMeshProvider(const std::vector<float>& vertices, const std::vector<unsigned int>& indices, const MeshCreateParams& params):
    vertices(vertices),
    indices(indices),
    MeshProvider(params)
{
}

std::pair<std::vector<VertexScalarType>, std::vector<unsigned int>> RawMeshProvider::GetMesh() const
{
    return std::make_pair(vertices, indices);
}

TextMeshProvider::TextMeshProvider(const MeshCreateParams& params, const std::shared_ptr<Material>& f) : MeshProvider(params), font(f)
{
}

unsigned int MeshVertexFormat::GetInstancedVertexSize() const {
    return instancedVertexSize;
}

unsigned int MeshVertexFormat::GetNonInstancedVertexSize() const {
    return noninstancedVertexSize;
}

MeshVertexFormat::MeshVertexFormat(std::vector<VertexAttribute> attribs) : attributes(attribs) {

    unsigned numModelMatrices = 0;

    // calculate attribute offsets and sizes
    unsigned int noninstancedOffset = 0, instancedOffset = 0;
    
    for (auto& attrib : attributes) {
        if (attrib.instanced) {
            if (attrib.writeModelMatrix) {
                numModelMatrices++;
                Assert(attrib.nComponents == 16);
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

    Assert(numModelMatrices == 1);
}

const std::vector<VertexAttribute>& MeshVertexFormat::GetAttributes() const {
    return attributes;
}


MeshVertexFormat MeshVertexFormat::Default(unsigned int nBones, bool instancedColor, bool instancedTextureZ) {
    bool animations = nBones != 0;
    auto format = MeshVertexFormat({
        .position = VertexAttribute {.nFloats = 3, .instanced = false},
        .textureUV = VertexAttribute {.nFloats = 2, .instanced = false},
        .textureZ = VertexAttribute {.nFloats = 1, .instanced = instancedTextureZ},
        .color = VertexAttribute {.nFloats = 4, .instanced = instancedColor},
        .modelMatrix = VertexAttribute {.nFloats = 16, .instanced = true},
        .normalMatrix = VertexAttribute {.nFloats = 9, .instanced = true},
        .normal = VertexAttribute {.nFloats = 3, .instanced = false},
        .tangent = VertexAttribute {.nFloats = 3, .instanced = false},
        .arbitrary1 = animations ? std::optional(VertexAttribute {.nFloats = 4, .instanced = false, .integer = true}) : std::nullopt, // bone ids
        .arbitrary2 = animations ? std::optional(VertexAttribute {.nFloats = 4, .instanced = false}) : std::nullopt, // bone weights
        }, animations, nBones);
    //format.primitiveType = GL_POINTS;
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



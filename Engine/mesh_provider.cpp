#include "mesh_provider.hpp"
#include "mesh.hpp"
#include <tiny_obj_loader.h>

MeshCreateParams MeshCreateParams::Default() {
    MeshCreateParams p;
    return std::move(p);
}

MeshCreateParams MeshCreateParams::DefaultGui() {
    MeshCreateParams p;
    p.meshVertexFormat = MeshVertexFormat::DefaultGui();
    return std::move(p);
}

void MeshCreateParams::LoadObj(std::string path) {
    
    tinyobj::ObjReaderConfig reader_config;
    tinyobj::ObjReader reader;
    bool success = reader.ParseFromFile(path, reader_config);
    if (!reader.Warning().empty()) DebugLogInfo("tinyobjloader warning: ", reader.Warning());
    if (!reader.Error().empty()) DebugLogError("tinyobjloader error: ", reader.Error());
    if (!success) {
        throw std::runtime_error("tinyobjloader error");
    }

    auto attrib = reader.GetAttrib(); // attrib contains our vertices
    vertices.resize(attrib.vertices.size() / 3 * meshVertexFormat.ScalarsPerVertex(), VertexScalar{ .u = 0xdddddddd });
    if (auto posAttribute = meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_POSITION))
        for (unsigned i = 0; i < attrib.vertices.size() / 3; i++) {
            memcpy(&vertices[i * meshVertexFormat.ScalarsPerVertex() + posAttribute->offset / sizeof(VertexScalar)], &attrib.vertices[i * 3], sizeof(VertexScalar) * 3);
        }
    if (auto normalsAttribute = meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_NORMAL))
        for (unsigned i = 0; i < attrib.normals.size() / 3; i++) {
            memcpy(&vertices[i * meshVertexFormat.ScalarsPerVertex() + normalsAttribute->offset / sizeof(VertexScalar)], &attrib.normals[i * 3], sizeof(VertexScalar) * 3);
        }
    if (auto uvsAttribute = meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_UV))
        for (unsigned i = 0; i < attrib.texcoords.size() / 2; i++) {
            memcpy(&vertices[i * meshVertexFormat.ScalarsPerVertex() + uvsAttribute->offset / sizeof(VertexScalar)], &attrib.texcoords[i * 2], sizeof(VertexScalar) * 2);
        }

    auto shapes = reader.GetShapes(); // shapes contain our indices
    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            Assert(fv == 3);
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                Assert(idx.vertex_index == idx.texcoord_index && idx.vertex_index == idx.normal_index);
                indices.push_back(idx.vertex_index);
            }
            index_offset += fv;

        }
    }
}

//MeshCreateParams& MeshCreateParams::operator=(TextMeshCreateParams&& other) noexcept {
//    return *this;
//}


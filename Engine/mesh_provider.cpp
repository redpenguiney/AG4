#include "mesh_provider.hpp"
#include "mesh.hpp"
#include "tiny_obj_loader.h"
#include "let_me_hash_a_tuple.hpp"
#include <tuple>

MeshCreateParams MeshCreateParams::Default() {
    MeshCreateParams p;
    return p;
}

MeshCreateParams MeshCreateParams::DefaultGui() {
    MeshCreateParams p;
    p.meshVertexFormat = MeshVertexFormat::DefaultGui();
    return p;
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

    unsigned scalarsPerVert = meshVertexFormat.ScalarsPerVertex();

    auto attrib = reader.GetAttrib(); // attrib contains our vertices (however, it stores positions/normals/etc in seperate arrays which share values to save data so its not that shrimple to load). 
    std::unordered_map<std::tuple<int, int, int>, unsigned, hash_tuple::hash<std::tuple<int,int,int>>> uniqueVerts;
    auto shapes = reader.GetShapes(); // shapes contain our indices
    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            Assert(fv == 3);
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                auto idxTuple = std::make_tuple(idx.vertex_index, idx.normal_index, idx.texcoord_index);
                if (!uniqueVerts.contains(idxTuple)) {

                    unsigned meshIdx = vertices.size() / meshVertexFormat.ScalarsPerVertex();
                    uniqueVerts[idxTuple] = meshIdx;

                    // guarantee that resize() call doesn't reallocate vector memory every time
                    if (vertices.capacity() < meshIdx + 1) {
                        vertices.reserve(2 * vertices.capacity());
                    }
                    unsigned baseMeshIndex = vertices.size();
                    vertices.resize(vertices.size() + meshVertexFormat.ScalarsPerVertex());

                    if (auto posInfo = meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_POSITION)) {
                        memcpy(&vertices[baseMeshIndex + posInfo->offset / sizeof(VertexScalar)], &attrib.vertices[3 * idx.vertex_index], 3 * sizeof(VertexScalar));
                    }
                    if (auto normalInfo = meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_NORMAL)) {
                        Assert(idx.normal_index >= 0);
                        memcpy(&vertices[baseMeshIndex + normalInfo->offset / sizeof(VertexScalar)], &attrib.normals[3 * idx.normal_index], 3 * sizeof(VertexScalar));
                    }
                    if (auto uvInfo = meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_UV)) {
                        Assert(idx.texcoord_index >= 0);
                        memcpy(&vertices[baseMeshIndex + uvInfo->offset / sizeof(VertexScalar)], &attrib.texcoords[2 * idx.texcoord_index], 2 * sizeof(VertexScalar));
                    }
                }
                indices.push_back(uniqueVerts[idxTuple]);
            }
            index_offset += fv;

        }
    }

    vertices.reserve(vertices.size());
}

void MeshCreateParams::LoadText(Texture& fontmap, std::string text, TextFormatting formatting) {
    // text wrapping
    if (formatting.wrapping) {

        // divide the text into words
        std::vector<std::string> words; // spaces, tabs, and newlines are words, ok?
        std::string currentWord = "";

        for (char& c : text) {
            if (c == ' ' || c == '\n' || c == '\t') {
                if (currentWord.length() > 0) words.push_back(currentWord);
                if (c == '\t') {
                    for (unsigned i = 0; i < formatting.tabLength; i++) words.push_back(" ");
                }
                else {
                    words.push_back(std::string(1, c));
                }
                currentWord = "";
            }
            else {
                currentWord += c;
            }
        }
        if (currentWord.length() > 0) words.push_back(currentWord);

        // then, for each word, see if it fits on the current line. 
            //  If it's just a newline/space/tab, duh.
            //  If it's bigger than maxLengthPerLine, split the word as needed. 
            //  Else, if it's bigger than  maxLengthPerLine - currentLineLegnth, put the word on the next line.
            //  Else, it fits on the current line, just append it directly to wrappedText (with a space)
        int currentLineLength = 0;
        int maxLineLength = formatting.rightMargin - formatting.leftMargin;
        Assert(maxLineLength > 0);
        std::string wrappedText;

        for (auto it = words.begin(); it != words.end(); it++) {
            auto& word = *it;

            if (word == "\n") {

            }
        }
    }
}

//MeshCreateParams& MeshCreateParams::operator=(TextMeshCreateParams&& other) noexcept {
//    return *this;
//}


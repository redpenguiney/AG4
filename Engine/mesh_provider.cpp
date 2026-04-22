#include "mesh_provider.hpp"
#include "mesh.hpp"
#include "tiny_obj_loader.h"
#include "let_me_hash_a_tuple.hpp"
#include <tuple>
#include "texture.hpp"

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
    if (text.empty()) return;

    Assert(fontmap.glyphs.has_value());

    std::vector<std::string> lines;
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
            //  If it's just a newline, start a new line.
            //  Spaces always fit.
            //  If it's bigger than maxLengthPerLine, split the word as needed. 
            //  Else, if it's bigger than  maxLengthPerLine - currentLineLength, this line is over; the word can go on the next line.
            //  Else, it fits on the current line, just append it directly to wrappedText. 
        int currentLineLength = 0; // in pixels
        int maxLineLength = formatting.rightMargin - formatting.leftMargin;
        Assert(maxLineLength > 0);
        std::string currentLine = "";
        for (auto it = words.begin(); it != words.end(); it++) {
            auto& word = *it;

            if (word == "\n") {
                lines.push_back(currentLine);
                currentLine = "";
                currentLineLength = 0;
            }
            else if (word == " ") {
                currentLineLength += fontmap.glyphs->at(' ').advance;
            }
            else {
                int wordLength; // in pixels
                unsigned i = 0;
                for (const char& c : word) {
                    wordLength += fontmap.glyphs->at(c).advance;

                    if (wordLength > maxLineLength /*&& currentLineLength == 0*/) {
                        // split the string and add newline
                        // std::cout << "Splitting word " << word << " into " << word.substr(0, i) << " and " << word.substr(i, std::string::npos) << ".\n";

                        currentLine += word.substr(0, i);
                        words.insert(std::next(it), word.substr(i, std::string::npos));
                        words.insert(std::next(it), "\n");
                        goto splitWord;
                    }

                    i++;
                }

                if (wordLength > (maxLineLength - currentLineLength)) {
                    words.insert(std::next(it), "\n");
                }
                else {
                    currentLine += word;
                    currentLineLength += wordLength;
                }

                splitWord:;
            }
        }
        if (currentLine.length() > 0) lines.push_back(currentLine);
    }
    else {
        std::string currentLine = "";

        for (char& c : text) {
            if (c == '\n') {
                lines.push_back(currentLine);
                currentLine = "";
            }
            else if (c == '\t') {
                for (unsigned i = 0; i < formatting.tabLength; i++) currentLine += " ";
            }
            else {
                currentLine += c;
            }
        }
    }

    // trim whitespace off each line
    for (auto &l : lines) {
        if (formatting.horizontalAlignment != HorizontalAlignMode::Right) {
            while (!l.empty() && l.back() == ' ') l.pop_back();
        }
    }

    // find line widths in pixels
    std::vector<int> lineLengths;
    for (unsigned i = 0; i < lines.size(); i++) {
        lineLengths.push_back(0);
        for (auto& c : lines[i]) {
            lineLengths[i] += fontmap.glyphs->at(c).advance;
        }
    }

    // Figure out height of first line
    if (lines.empty()) DebugLogError("No text???"); return;
    int textHeight = formatting.lineSpacing * (lines.size() - 1); // distance between baseline of first line and baseline of last line.
    int topLineHeight = 0; // top line baseline to top of letter with greatest yMax
    int bottomLineHeight = 0; // bottom line baseline to bottom of letter with least yMin
    for (auto& c : lines[0]) {
        topLineHeight = std::max(topLineHeight, fontmap.glyphs->at(c).bearingY);
    }
    for (auto& c : lines.back()) {
        bottomLineHeight = std::min(bottomLineHeight, static_cast<int>(fontmap.glyphs->at(c).height) - fontmap.glyphs->at(c).bearingY);
    }
    int totalHeight = textHeight + topLineHeight + bottomLineHeight;

    int currentY;
    if (formatting.verticalAlignment == VerticalAlignMode::Top) {
        currentY = formatting.topMargin - topLineHeight;
    }
    else if (formatting.verticalAlignment == VerticalAlignMode::Center) {
        int desiredCentre = (formatting.topMargin - formatting.bottomMargin) / 2;
        currentY = desiredCentre + (textHeight + topLineHeight + bottomLineHeight) / 2 - topLineHeight;
    }
    else {
        currentY = formatting.bottomMargin + bottomLineHeight;
    }

    // Write vertices
    unsigned vertexIndex = 0;
    unsigned scalarsPerVertex = meshVertexFormat.ScalarsPerVertex();
    auto& posAttrib = *meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_POSITION);
    unsigned posOffset = posAttrib.offset / sizeof(VertexScalar);
    auto& uvsAttrib = *meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_UV);
    unsigned uvsOffset = posAttrib.offset / sizeof(VertexScalar);
    for (unsigned i = 0; i < lines.size(); i++) {

        // Figure out horizontal position of first letter of the line
        int currentX; 
        if (formatting.horizontalAlignment == HorizontalAlignMode::Left) {
            currentX = formatting.leftMargin;
        }
        else if (formatting.horizontalAlignment == HorizontalAlignMode::Right) {
            currentX = formatting.rightMargin - lineLengths[i];
        }
        else {
            currentX = -lineLengths[i] / 2;
        }

        for (auto& c : lines[i]) {
            if (c == ' ') {
                currentX += formatting.spaceLength;
            }
            else {
                vertices.resize(vertices.size() + 4 * scalarsPerVertex);
                indices.resize(indices.size() + 6);

                int width = fontmap.glyphs->at(c).width;
                int height = fontmap.glyphs->at(c).height;
                int startX = fontmap.glyphs->at(c).bearingX;
                int startY = fontmap.glyphs->at(c).bearingY;

                indices.push_back(vertexIndex + 2);
                indices.push_back(vertexIndex + 1);
                indices.push_back(vertexIndex);
                indices.push_back(vertexIndex + 3);
                indices.push_back(vertexIndex + 2);
                indices.push_back(vertexIndex);

                vertices[vertexIndex * scalarsPerVertex + posOffset + 0] = startX;
                vertices[vertexIndex * scalarsPerVertex + posOffset + 1] = startY;
                vertices[vertexIndex * scalarsPerVertex + uvsOffset + 0] = fontmap.glyphs->at(c).leftUv;
                vertices[vertexIndex * scalarsPerVertex + uvsOffset + 1] = fontmap.glyphs->at(c).bottomUv;
                vertexIndex += 1;
                vertices[vertexIndex * scalarsPerVertex + posOffset + 0] = startX;
                vertices[vertexIndex * scalarsPerVertex + posOffset + 1] = startY + height;
                vertices[vertexIndex * scalarsPerVertex + uvsOffset + 0] = fontmap.glyphs->at(c).leftUv;
                vertices[vertexIndex * scalarsPerVertex + uvsOffset + 1] = fontmap.glyphs->at(c).topUv;
                vertexIndex += 1;
                vertices[vertexIndex * scalarsPerVertex + posOffset + 0] = startX + width;
                vertices[vertexIndex * scalarsPerVertex + posOffset + 1] = startY + height;
                vertices[vertexIndex * scalarsPerVertex + uvsOffset + 0] = fontmap.glyphs->at(c).rightUv;
                vertices[vertexIndex * scalarsPerVertex + uvsOffset + 1] = fontmap.glyphs->at(c).topUv;
                vertexIndex += 1;
                vertices[vertexIndex * scalarsPerVertex + posOffset + 0] = startX + width;
                vertices[vertexIndex * scalarsPerVertex + posOffset + 1] = startY;
                vertices[vertexIndex * scalarsPerVertex + uvsOffset + 0] = fontmap.glyphs->at(c).rightUv;
                vertices[vertexIndex * scalarsPerVertex + uvsOffset + 1] = fontmap.glyphs->at(c).bottomUv;
                vertexIndex += 1;

                currentX += fontmap.glyphs->at(c).advance;
            }
        }

        currentY += formatting.lineSpacing;
    }
}

//MeshCreateParams& MeshCreateParams::operator=(TextMeshCreateParams&& other) noexcept {
//    return *this;
//}


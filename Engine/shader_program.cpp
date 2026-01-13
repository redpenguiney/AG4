#include "graphics_engine.hpp"
#include "shader_program.hpp"
#include "assert.hpp"
#include <cstring>
#include <memory>
#include <string>
#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "GL/glew.h"
#include "glm/glm.hpp"
//#include <optional>

// TODO: custom exceptions



const std::vector<ShaderActiveVertexAttribute>& ShaderProgram::GetInputVertexAttributes() {
    return inputVertexAttributes;
}

void ShaderProgram::SetCameraUniforms(glm::mat4x4 cameraProjMatrix, glm::mat4x4 cameraProjMatrixNoFloatingOrigin, glm::mat4x4 orthrographicMatrix) {
    for (auto & [shaderId, shaderProgram] : LOADED_SHADER_PROGRAMS) {
        (void)shaderId;
        if (shaderProgram->HasUniform("orthro")) { // make sure the shader program actually wants our camera/projection matrix
            shaderProgram->Uniform("orthro", orthrographicMatrix, false);
        }
        if (shaderProgram->HasUniform("perspective")) {
            shaderProgram->Uniform("perspective", (shaderProgram->useFloatingOrigin) ? cameraProjMatrix : cameraProjMatrixNoFloatingOrigin, false);
        }
    }
}

std::shared_ptr<ShaderProgram> ShaderProgram::New(const char* vertexPath, const char* fragmentPath, const bool floatingOrigin, const bool useLightClusters) {
    auto ptr = std::shared_ptr<ShaderProgram>(new ShaderProgram(vertexPath, fragmentPath, floatingOrigin, useLightClusters));
    LOADED_PROGRAMS.emplace(ptr->shaderProgramId, ptr);
    LOADED_SHADER_PROGRAMS.emplace(ptr->shaderProgramId, ptr);
    return ptr;
}

std::shared_ptr<ShaderProgram> ShaderProgram::Get(unsigned int shaderProgramId) {
    Assert(LOADED_SHADER_PROGRAMS.count(shaderProgramId) != 0 && "ShaderProgram::Get() was given an invalid shaderProgramId.");
    //auto ptr = std::dynamic_pointer_cast<ShaderProgram>(LOADED_PROGRAMS[shaderProgramId]); // mwahahaha
    //Assert(ptr != nullptr); // make sure this is a pointer to a ShaderProgram, not a ComputeShaderProgram/etc.
    return LOADED_SHADER_PROGRAMS[shaderProgramId];
}



void ShaderProgram::Unload(unsigned int id)
{
    //Assert(!GraphicsEngine::Get().IsShaderProgramInUse(id)); // don't let them unload a shader program if it's being used
    Assert(LOADED_PROGRAMS.count(id) != 0 && "ShaderProgram::Unload() was given an invalid shaderProgramId.");
    LOADED_PROGRAMS.erase(id);
    BaseShaderProgram::Unload(id);
}

ShaderProgram::~ShaderProgram() {
    //glDeleteProgram(programId);
}

std::string ShaderProgram::GetVertexSourcePath() {
    return vertex.path;
}





ShaderProgram::ShaderProgram(const char* vertexPath, const char* fragmentPath, const bool floatingOrigin, const bool useLightClusters):
    BaseShaderProgram(),
    useFloatingOrigin(floatingOrigin),
    useClusteredLighting(useLightClusters),
    vertex(vertexPath, GL_VERTEX_SHADER),
    fragment(fragmentPath, GL_FRAGMENT_SHADER)
{

    // attach shaders to program
    glAttachShader(shaderProgramId, vertex.shaderId);
    glAttachShader(shaderProgramId, fragment.shaderId);

    glBindFragDataLocation(shaderProgramId, 0, "color"); // tell opengl that the variable we're putting the final pixel color in is called "color"

    Link();

    int nAttribs, longestAttribName;
    glGetProgramiv(shaderProgramId, GL_ACTIVE_ATTRIBUTES, &nAttribs);
    glGetProgramiv(shaderProgramId, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &longestAttribName);
    Assert(nAttribs > 0);
    auto nameBuffer = new char[longestAttribName+1];
    for (unsigned i = 0; i < nAttribs; i++) {
        GLsizei nameLength = -1;
        GLint attribSize;
        GLenum attribType;
        glGetActiveAttrib(shaderProgramId, i, longestAttribName, &nameLength, &attribSize, &attribType, nameBuffer);

        if (nameLength != -1) {
            inputVertexAttributes.push_back(ShaderActiveVertexAttribute{
                .index = i,
                .name = std::string(nameBuffer, nameLength),
                .type = attribType
            });
        }
    }
    delete[] nameBuffer;
}
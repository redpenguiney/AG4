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
    for (auto & [shaderId, shaderProgram] : LOADED_PROGRAMS) {
        (void)shaderId;
        if (shaderProgram->HasUniform("orthro")) { // make sure the shader program actually wants our camera/projection matrix
            shaderProgram->Uniform("orthro", orthrographicMatrix, false);
        }
        if (shaderProgram->HasUniform("perspective")) {
            shaderProgram->Uniform("perspective",  cameraProjMatrix, false);
        }
        if (shaderProgram->HasUniform("persectiveNoFloatingOrigin")) {
            shaderProgram->Uniform("perspectiveNoFloatingOrigin", cameraProjMatrixNoFloatingOrigin, false);
        }
    }
}

std::shared_ptr<ShaderProgram> ShaderProgram::New(const char* vertexPath, const char* fragmentPath) {
    auto ptr = std::shared_ptr<ShaderProgram>(new ShaderProgram(vertexPath, fragmentPath));
    LOADED_PROGRAMS.emplace(ptr->shaderProgramId, ptr);
    return ptr;
}

std::shared_ptr<ShaderProgram> ShaderProgram::Get(unsigned int shaderProgramId) {
    Assert(LOADED_PROGRAMS.count(shaderProgramId) != 0 && "ShaderProgram::Get() was given an invalid shaderProgramId.");
    Assert(dynamic_pointer_cast<ShaderProgram>(LOADED_PROGRAMS[shaderProgramId]) != nullptr);
    return dynamic_pointer_cast<ShaderProgram>(LOADED_PROGRAMS[shaderProgramId]);
}

ShaderProgram::~ShaderProgram() {
    //glDeleteProgram(programId);
}

std::string ShaderProgram::GetVertexSourcePath() {
    return vertex.path;
}



unsigned NumArraysFromGLAttributeType(GLenum type) {
    switch (type) {
    case GL_FLOAT:
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT_VEC2:
    case GL_INT_VEC2:
    case GL_UNSIGNED_INT_VEC2:
    case GL_FLOAT_VEC3:
    case GL_INT_VEC3:
    case GL_UNSIGNED_INT_VEC3:
    case GL_FLOAT_VEC4:
    case GL_INT_VEC4:
    case GL_UNSIGNED_INT_VEC4:
        return 1;
    case GL_FLOAT_MAT3:
        return 3;
    case GL_FLOAT_MAT4:
        return 4;
    default:
        Assert(false);
    }
}

unsigned NumComponentsPerArrayFromGLAttributeType(GLenum type) {
    switch (type) {
    case GL_FLOAT:
    case GL_INT:
    case GL_UNSIGNED_INT:
        return 1;
    case GL_FLOAT_VEC2:
    case GL_INT_VEC2:
    case GL_UNSIGNED_INT_VEC2:
        return 2;
    case GL_FLOAT_VEC3:
    case GL_INT_VEC3:
    case GL_UNSIGNED_INT_VEC3:
        return 3;
    case GL_FLOAT_VEC4:
    case GL_INT_VEC4:
    case GL_UNSIGNED_INT_VEC4:
        return 4;
    case GL_FLOAT_MAT3:
        return 3;
    case GL_FLOAT_MAT4:
        return 4;
    default:
        Assert(false);
    }
}

VertexScalarType ScalarTypeFromGLAttributeType(GLenum type) {
    switch (type) {
    case GL_FLOAT:
    case GL_FLOAT_VEC2:
    case GL_FLOAT_VEC3:
    case GL_FLOAT_VEC4:
    case GL_FLOAT_MAT3:
    case GL_FLOAT_MAT4:
        return VertexScalarType::f32;
    case GL_INT:
    case GL_INT_VEC2:
    case GL_INT_VEC3:
    case GL_INT_VEC4:
        return VertexScalarType::i32;
    case GL_UNSIGNED_INT:
    case GL_UNSIGNED_INT_VEC2:
    case GL_UNSIGNED_INT_VEC3:
    case GL_UNSIGNED_INT_VEC4:
        return VertexScalarType::u32;
    default:
        Assert(false);
    }
}

ShaderProgram::ShaderProgram(const char* vertexPath, const char* fragmentPath):
    BaseShaderProgram(),
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
    for (int i = 0; i < nAttribs; i++) {
        GLsizei nameLength = -1;
        GLint attribSize;
        GLenum attribType;
        glGetActiveAttrib(shaderProgramId, i, longestAttribName, &nameLength, &attribSize, &attribType, nameBuffer);
        Assert(attribSize == 1); // we don't support array type attributes

        if (nameLength != -1) {
            int bindingLocation = glGetAttribLocation(shaderProgramId, nameBuffer);
            Assert(bindingLocation != -1);
            inputVertexAttributes.push_back(ShaderActiveVertexAttribute{
                .index = unsigned(bindingLocation),
                .name = std::string(nameBuffer, nameLength),
                .type = attribType,
                .scalarType = ScalarTypeFromGLAttributeType(attribType),
                .nArrays = NumArraysFromGLAttributeType(attribType),
                .nComponentsPerArray = NumComponentsPerArrayFromGLAttributeType(attribType)
            });
        }
    }
    delete[] nameBuffer;

    for (auto& [name, uniform] : shaderUniforms) {
        if (name.length() > strlen(AUTO_TEXTURE_ARRAY_UNIFORM_SUFFIX) && name.substr(name.length() - strlen(AUTO_TEXTURE_ARRAY_UNIFORM_SUFFIX)) == AUTO_TEXTURE_ARRAY_UNIFORM_SUFFIX) {
            for (auto& attrib : inputVertexAttributes) {
                if (attrib.name == SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION) goto allGood;
            }
            throw std::runtime_error("The shader requested an autoarray texture " + name + " but not the corresponding vertex attribute for array texture layer selection.");
        allGood:;
        }
    }

    for (auto& [name, uniform] : shaderUniforms) {
        if (shaderUniforms.contains(name + AUTO_TEXTURE_ARRAY_UNIFORM_SUFFIX)) {
            throw std::runtime_error("For texture " + name + " there was also an autoarray version of the texture found. Remove one of them.");
        }
    }
}
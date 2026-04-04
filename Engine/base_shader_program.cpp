#include <string>
#include "base_shader_program.hpp"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include "assert.hpp"
#include <vector>
#include "mesh.hpp"
#include "graphics_engine.hpp"

using namespace std::string_literals;

static std::string LoadFile(const char* path) {
    std::ifstream stream(path);
    if (stream.fail()) { // Verify file was successfully found/open
        throw std::runtime_error("Unable to find or read the file at path "s + path + " during shader compilation."s);
    }
    return { std::istreambuf_iterator<char>(stream), {} };
}

std::string Shader::PreprocessFile(std::string filepath)
{
    // TODO: this sucks; ultimately we gonna have to find a portable way to do shaders
    if (fileCache.contains(filepath)) {
        return fileCache[filepath];
    }
    else {
        std::string unprocessedSource = LoadFile(filepath.c_str());
        std::string preprocessedSource = "";
        std::istringstream iss(unprocessedSource);

        unsigned int lineNum = 1;
        for (std::string line; std::getline(iss, line); lineNum++)
        {
            std::string::size_type lineCommentIndex = line.find("//");
            std::string::size_type includeFileIndex = line.find("#include");

            if (includeFileIndex == std::string::npos || lineCommentIndex < includeFileIndex) {
                preprocessedSource += line + "\n";
            }
            else if (lineCommentIndex > includeFileIndex) {
                std::string::size_type firstQuoteIndex = line.find("\"");
                if (firstQuoteIndex == std::string::npos || firstQuoteIndex > line.length() - 1) {
                    throw std::runtime_error("Invalid include statement at " + std::to_string(lineNum) + ": no opening \" found");
                }

                std::string pathsubStr = line.substr(firstQuoteIndex + 1);

                std::string::size_type secondQuoteIndex = pathsubStr.find("\"");
                if (secondQuoteIndex == std::string::npos || secondQuoteIndex > line.length() - 1) {
                    throw std::runtime_error("Invalid include statement at " + std::to_string(lineNum) + ": perhaps your closing \" is missing?");
                }

                pathsubStr = pathsubStr.substr(0, secondQuoteIndex);
                //DebugLogInfo("Including file ", pathsubStr);
                preprocessedSource += PreprocessFile(pathsubStr) + "\n";
            }
        }

        fileCache[filepath] = preprocessedSource;
        return preprocessedSource;
    }
}

std::string Shader::GetInfoLog() {
    int InfoLogLength = 0;
    int CharsWritten = 0;

    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &InfoLogLength);

    if (InfoLogLength > 0)
    {
        GLchar* InfoLog = new GLchar[InfoLogLength];
        glGetShaderInfoLog(shaderId, InfoLogLength, &CharsWritten, InfoLog);
        return std::string(InfoLog);
        delete[] InfoLog;
    };

    Assert(false);
    std::unreachable();
}

Shader::Shader(const char* path, GLenum shaderType):
    path(path)
{
    // Get string from file contents 
    std::string mainShaderSource = PreprocessFile(path);
    
    const char* mainShaderSourcePtr = mainShaderSource.c_str();
    GLint mainSourceLength = mainShaderSource.length();

    // tell opengl about the files the shader source wants to include
    //Assert(includedFiles.empty()); // openGL support for #include is a sham, we're gonna have to add support ourselves at some point
    // for (auto & includePath: includedFiles) {
    //     std::string source = LoadFile(includePath);
    //     const char* sourcePtr = source.c_str();
    //     GLint sourceLength = source.length();
    //     glNamedStringARB(GL_SHADER_INCLUDE_ARB, strlen(includePath), includePath, sourceLength, sourcePtr);
    // }

    // Compile the shader
    GLint compiled;
    shaderId = glCreateShader(shaderType);
    glShaderSource(shaderId, 1, &mainShaderSourcePtr, &mainSourceLength);
    glCompileShader(shaderId);
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        std::string compilationFailure = "Failed to compile \""s + path + "\". GLSL compiler log (line numbers may be inaccurate due to include statements):\n" + GetInfoLog();
        DebugLogError(compilationFailure);
        throw std::runtime_error(compilationFailure);
    };
}

Shader::~Shader() {
    glDeleteShader(shaderId);
}

void BaseShaderProgram::Unload(unsigned int id) {
    // meshpools hold onto a shared_ptr to the shader so its fine
    //Assert(!GraphicsEngine::Get().IsShaderProgramInUse(id)); // don't let them unload a shader program if it's being used
    Assert(LOADED_PROGRAMS.count(id) != 0 && "BaseShaderProgram::Unload() was given an invalid shaderProgramId.");
    LOADED_PROGRAMS.erase(id);
}

BaseShaderProgram::~BaseShaderProgram()
{
    glDeleteProgram(programId);
}

BaseShaderProgram::BaseShaderProgram()
{
    // Create shader program and attach vertex/fragment to it
    programId = glCreateProgram();
}

const std::unordered_map<std::string, ShaderUniformInfo>& BaseShaderProgram::GetUniforms() {
    return shaderUniforms;
}

const std::vector<ShaderStorageBufferInfo>& BaseShaderProgram::GetShaderSSBOs()
{
    return shaderSSBOs;
}

const std::vector<ShaderUniformBufferInfo>& BaseShaderProgram::GetShaderUBOs()
{
    return shaderUBOs;
}

bool BaseShaderProgram::HasUniform(std::string name)
{
    return shaderUniforms.contains(name);
}

void BaseShaderProgram::Link()
{
    glLinkProgram(shaderProgramId);
    int success;
    char infolog[512];
    glGetProgramiv(shaderProgramId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgramId, 512, NULL, infolog);
        printf("\nFailed to link shader program!\n%s\n", infolog);
        abort();
    }

    GLint numUniforms, longestUniformName;
    glGetProgramiv(shaderProgramId, GL_ACTIVE_UNIFORMS, &numUniforms);
    glGetProgramiv(shaderProgramId, GL_ACTIVE_UNIFORM_MAX_LENGTH, &longestUniformName);
    for (int i = 0; i < numUniforms; i++) {
        auto nameBuf = new char[longestUniformName + 1];
        GLsizei length;
        GLint size;
        GLenum type;
        glGetActiveUniform(shaderProgramId, i, longestUniformName, &length, &size, &type, nameBuf);
        std::string name(nameBuf);
        Assert(size == 1); // TODO: should probably support array uniforms
        auto location = glGetUniformLocation(shaderProgramId, nameBuf);
        if (location != -1)
            shaderUniforms[name] = ShaderUniformInfo{
                .name = name,
                .uniformLocation = location,
                .type = type
            };
        // doesn't work because UBO members were being treated as uniforms
        //Assert(shaderUniforms[name].uniformLocation != -1);

        delete[] nameBuf;
    }

    GLint numSSBOs, numUBOs, ssboNameLength, uboNameLength;
    glGetProgramInterfaceiv(shaderProgramId, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &numSSBOs);
    glGetProgramInterfaceiv(shaderProgramId, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &numUBOs);
    glGetProgramInterfaceiv(shaderProgramId, GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH, &ssboNameLength);
    glGetProgramInterfaceiv(shaderProgramId, GL_UNIFORM_BLOCK, GL_MAX_NAME_LENGTH, &uboNameLength);

    for (int i = 0; i < numSSBOs; i++) {
        auto nameBuf = new char[ssboNameLength + 1];
        GLsizei nameLen;
        glGetProgramResourceName(shaderProgramId, GL_SHADER_STORAGE_BLOCK, i, ssboNameLength, &nameLen, nameBuf);
        std::string name(nameBuf, nameLen);
        delete[] nameBuf;

        shaderSSBOs.push_back(ShaderStorageBufferInfo {
            .name = name,
            .index = i
            });
    }

    for (int i = 0; i < numUBOs; i++) {
        auto nameBuf = new char[uboNameLength + 1];
        GLsizei nameLen;
        glGetProgramResourceName(shaderProgramId, GL_UNIFORM_BLOCK, i, uboNameLength, &nameLen, nameBuf);
        std::string name(nameBuf, nameLen);
        delete[] nameBuf;

        shaderUBOs.push_back(ShaderUniformBufferInfo{
            .name = name,
            .index = i,
            });
    }
}

void BaseShaderProgram::Uniform(std::string uniformName, glm::mat4x4 matrix, bool transposeMatrix, bool require) {
    if (shaderUniforms.count(uniformName) == 0) {
        if (require) throw std::runtime_error("Shader has no uniform " + uniformName);
        else return;
    }
    if (shaderUniforms[uniformName].type != GL_FLOAT_MAT4) {
        if (require) throw std::runtime_error("Shader expects a different type for this uniform.");
        else return;
    }
    Use();
    glUniformMatrix4fv(shaderUniforms.at(uniformName).uniformLocation, 1, transposeMatrix, &matrix[0][0]);
}

void BaseShaderProgram::Uniform(std::string uniformName, glm::vec4 vec, bool require) {
    if (shaderUniforms.count(uniformName) == 0) {
        if (require) throw std::runtime_error("Shader has no uniform " + uniformName);
        else return;
    };
    if (shaderUniforms[uniformName].type != GL_FLOAT_VEC4) {
        if (require) throw std::runtime_error("Shader expects a different type for this uniform.");
        else return;
    }
    Use();
    glUniform4fv(shaderUniforms.at(uniformName).uniformLocation, 1, &vec.x);
}

void BaseShaderProgram::Uniform(std::string uniformName, glm::vec3 vec, bool require) {
    if (shaderUniforms.count(uniformName) == 0) {
        if (require) throw std::runtime_error("Shader has no uniform " + uniformName);
        else return;
    };
    if (shaderUniforms[uniformName].type != GL_FLOAT_VEC3) {
        if (require) throw std::runtime_error("Shader expects a different type for this uniform.");
        else return;
    }
    Use();
    glUniform3fv(shaderUniforms.at(uniformName).uniformLocation, 1, &vec.x);
}

void BaseShaderProgram::Uniform(std::string uniformName, glm::vec2 vec, bool require) {
    if (shaderUniforms.count(uniformName) == 0) {
        if (require) throw std::runtime_error("Shader has no uniform " + uniformName);
        else return;
    };
    if (shaderUniforms[uniformName].type != GL_FLOAT_VEC2) {
        if (require) throw std::runtime_error("Shader expects a different type for this uniform.");
        else return;
    }
    Use();
    glUniform2fv(shaderUniforms.at(uniformName).uniformLocation, 1, &vec.x);
}

void BaseShaderProgram::Uniform(std::string uniformName, float fval, bool require) {
    if (shaderUniforms.count(uniformName) == 0) {
        if (require) throw std::runtime_error("Shader has no uniform " + uniformName);
        else return;
    };
    if (shaderUniforms[uniformName].type != GL_FLOAT) {
        if (require) throw std::runtime_error("Shader expects a different type for this uniform.");
        else return;
    }
    Use();
    glUniform1f(shaderUniforms.at(uniformName).uniformLocation, fval);
}

void BaseShaderProgram::Uniform(std::string uniformName, bool bval, bool require) {
    if (shaderUniforms.count(uniformName) == 0) {
        if (require) throw std::runtime_error("Shader has no uniform " + uniformName);
        else return;
    };
    if (shaderUniforms[uniformName].type != GL_BOOL) {
        if (require) throw std::runtime_error("Shader expects a different type for this uniform.");
        else return;
    }
    Use();
    glUniform1i(shaderUniforms.at(uniformName).uniformLocation, bval);
}

void BaseShaderProgram::Uniform(std::string uniformName, unsigned int uval, bool require)
{
    if (shaderUniforms.count(uniformName) == 0) {
        if (require) throw std::runtime_error("Shader has no uniform " + uniformName);
        else return;
    };
    if (shaderUniforms[uniformName].type != GL_UNSIGNED_INT) {
        if (require) throw std::runtime_error("Shader expects a different type for this uniform.");
        else return;
    }
    Use();
    glUniform1ui(shaderUniforms.at(uniformName).uniformLocation, uval);
}

void BaseShaderProgram::Uniform(std::string uniformName, int ival, bool require) {
    if (shaderUniforms.count(uniformName) == 0) {
        if (require) throw std::runtime_error("Shader has no uniform " + uniformName);
        else return;
    };
    if (shaderUniforms[uniformName].type != GL_INT) {
        if (require) throw std::runtime_error("Shader expects a different type for this uniform.");
        else return;
    }
    Use();
    glUniform1i(shaderUniforms.at(uniformName).uniformLocation, ival);
}

void BaseShaderProgram::Use() {
    if (CURRENTLY_BOUND_PROGRAM_ID != programId) {
        glUseProgram(programId);
        CURRENTLY_BOUND_PROGRAM_ID = programId;
    }

   
}

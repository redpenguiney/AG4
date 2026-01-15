#pragma once
#include "base_shader_program.hpp"
#include <memory>

class ComputeShaderProgram: public BaseShaderProgram {
public:
    static std::shared_ptr<ComputeShaderProgram> New(const char* computePath);

    // WARNING: does no memory barriers! you must use some form  of synchronization after calling Dispatch()!
    void Dispatch(glm::uvec3 workgroupSize);

private:
    Shader shader;

    ComputeShaderProgram(const char* computePath);
};
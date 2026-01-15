#include "compute_shader_program.hpp"

std::shared_ptr<ComputeShaderProgram> ComputeShaderProgram::New(const char* computePath) {
    return std::shared_ptr<ComputeShaderProgram>(new ComputeShaderProgram(computePath));
}

void ComputeShaderProgram::Dispatch(glm::uvec3 workgroupSize) {
    Use();
    glDispatchCompute(workgroupSize.x, workgroupSize.y, workgroupSize.z);
}

ComputeShaderProgram::ComputeShaderProgram(const char* computePath): shader(computePath, GL_COMPUTE_SHADER) {
    glAttachShader(shaderProgramId, shader.shaderId);
    Link();
}
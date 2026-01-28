#pragma once
#include <memory>
#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>
#include "glm/glm.hpp"
#include "base_shader_program.hpp"
#include "mesh_format.hpp"

// TODO: custom procedural textures
// TODO: potentially big optimizations to be made with program pipeline objects

constexpr inline const char* AUTO_TEXTURE_ARRAY_UNIFORM_SUFFIX = "AUTO_ARRAY";

class ShaderProgram;

struct ShaderActiveVertexAttribute {
    unsigned index; // the index of the (first if nArrays>0) vertex attribute array you should activate in the vao to use with this attribute
    std::string name;
    GLenum type;
    VertexScalarType scalarType;
    unsigned nArrays;
    unsigned nComponentsPerArray;
};

// Shader program used for rendering, contains (at minimum) a vertex shader and a fragment (basically per pixel) shader.
class ShaderProgram: public BaseShaderProgram {
public:
    const std::vector<ShaderActiveVertexAttribute>& GetInputVertexAttributes();
    
    // const std::optional<Framebuffer> RenderTo; TODO: for stuff like viewport frames. things rendered with this shader will be rendered onto the framebuffer contained in the optional, or onto the main framebuffer if it's nullopt.

    // TODO: the graphics engine should really just do this itself. 
    // Passes projection/camera matrix to shaders that have useCameraMatrix == true.
    // Takes two different matrices, for shaders that don't use floating origin
    // Called by GraphicsEngine.
    static void SetCameraUniforms(glm::mat4x4 cameraProjMatrix, glm::mat4x4 cameraProjMatrixNoFloatingOrigin, glm::mat4x4 orthrographicMatrix);

    // Returns generated program.
    static std::shared_ptr<ShaderProgram> New(const char* vertexPath, const char* fragmentPath);

    // Creates a compute shader for performing arbitrary GPU calculations.
    // Returns id of generated program.
    //static std::shared_ptr<ShaderProgram> NewCompute(const char* computePath);

    // Get a pointer to a shader program by its id.
    static std::shared_ptr<ShaderProgram> Get(unsigned int shaderProgramId);

    ~ShaderProgram();

    std::string GetVertexSourcePath();

private:

    std::vector<ShaderActiveVertexAttribute> inputVertexAttributes; // TODO: offering default values

    
    Shader vertex; // processes each vertex 
    Shader fragment; // processes each fragment/pixel
    // TODO: tesselation, geometry shaders

    ShaderProgram(const char* vertexPath, const char* fragmentPath);
    //ShaderProgram(const char* computePath);

};
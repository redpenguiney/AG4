#include "mesh_provider.hpp"
#include "mesh.hpp"

MeshCreateParams MeshCreateParams::Default() {
    MeshCreateParams p;
    return std::move(p);
}

MeshCreateParams MeshCreateParams::DefaultGui() {
    MeshCreateParams p;
    p.meshVertexFormat = MeshVertexFormat::DefaultGui();
    return std::move(p);
}

//MeshCreateParams& MeshCreateParams::operator=(TextMeshCreateParams&& other) noexcept {
//    return *this;
//}


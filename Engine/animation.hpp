#pragma once
#include <string>
#include <vector>
#include <optional>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

struct Bone {
    std::string name;
    int parentIndex; // -1 if (a) root
    std::vector<int> childrenIndices;
    int index; // (into Mesh::bones, shader array, and vertex bone ids)
    glm::mat4x4 offsetMatrix; // mesh space to bone space (when the rig is t-posing or "in bind pose")
    glm::mat4x4 baseBoneTransform; // bone transform in model space (when in bind pose)
};

struct BoneKeyframe {
    glm::vec3 translation;
    glm::vec3 scale;
    glm::quat rotation;

    float timestamp; 
};

// Each Animation has a BoneAnimation for every bone it affects.
struct BoneAnimation {
    std::vector<BoneKeyframe> keyframes; // sorted from start to end. 
    unsigned int boneIndex; // index of the bone this animation affects
};

// Actual playing of animations is carried out by the graphics engine.
class Animation {
    public:
    std::string name;
    float duration;
    float priority; // higher number = higher priority; can be negative

    std::vector<BoneAnimation> boneAnimations; // one for every bone affected by the animation. In no particular order.

    // interpolates between keyframes.
    // this transform is relative to the bone's parent.
    // if the animation does not have a bone corresponding to this id, returns nullopt.
    std::optional<glm::mat4x4> BoneTransformAtTime(unsigned int boneId, float time) const;
};
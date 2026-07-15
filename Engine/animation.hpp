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
    glm::mat4x4 baseBoneTransform; // NOT the bone transform in model space (when in bind pose) because that's just the identity matrix. what is this???

    glm::vec3 direction;
};

struct PosKeyframe {
    glm::vec3 translation;
    float timestamp; 
};

struct RotKeyframe {
    glm::quat rotation;
    float timestamp;
};

struct ScaleKeyframe {
    glm::vec3 scale;
    float timestamp;
};

enum class SkeletonState {
    Clean, // no action needed
    Dirty, // bones have changed, need to update to new state. RenderScene() will reset this to Clean.
    StreamDirty, // bones have changed, but we're in the middle of an animation and their current transforms will only last for one frame, so stream them in. RenderScene() will leave state as StreamDirty.
};

// The bone transforms of a specific gameobject's instance.
struct Skeleton {
    glm::mat4x4* boneTransforms; // Owned by Skeleton.
    SkeletonState state;
    const unsigned nBoneTransforms;

    Skeleton(unsigned nBones);
    Skeleton(const Skeleton&) = delete;
    Skeleton(Skeleton&&);
    ~Skeleton();
};

// Each Animation has a BoneAnimation for every bone it affects.
struct BoneAnimation {
    // we store keyframes for each form of transformation seperately because that's how ASSIMP does it, it saves storage sometimes, and because supposedly it lets animations play nicely together more often.
    std::vector<PosKeyframe> positions; // sorted from start to end. 
    std::vector<RotKeyframe> rotations; // sorted from start to end. 
    std::vector<ScaleKeyframe> scalings; // sorted from start to end. 
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
    std::optional<glm::mat4x4> BoneTransformAtTime(unsigned int boneId, float time, bool looped) const;
};
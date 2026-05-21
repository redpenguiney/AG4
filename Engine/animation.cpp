#include "animation.hpp"
#include "log.hpp"
#include "assert.hpp"
#include "glm/gtx/quaternion.hpp"

std::optional<glm::mat4x4> Animation::BoneTransformAtTime(unsigned int boneId, float time, bool looped) const {

    // find the bone in boneAnimations
    const BoneAnimation* boneAnim = nullptr;
    for (auto& b : boneAnimations) {
        if (b.boneIndex == boneId) {
            boneAnim = &b;
            break;
        }
    }

    if (!boneAnim) { // then bone isn't affected by this animation: return nullopt
        return std::nullopt;
    }

    glm::vec3 interpolatedPos = boneAnim->positions.at(0).translation;
    glm::vec3 interpolatedScl = boneAnim->scalings.at(0).scale;
    glm::quat interpolatedRot = boneAnim->rotations.at(0).rotation;

    // for each transformation mode, find the two keyframes closest to the requested time and return them
    for (unsigned int i = looped ? 0 : 1; i < boneAnim->positions.size(); i++) {
        const auto& keyframe = boneAnim->positions[i];
        if (keyframe.timestamp > time) { // then this is the next keyframe we're going to reach.
            const auto& prevKeyframe = boneAnim->positions[(i == 0 ? boneAnim->positions.size() : i) - 1];

            float interpolationFactor = (time - prevKeyframe.timestamp) / (keyframe.timestamp - prevKeyframe.timestamp);
            interpolatedPos = glm::mix(prevKeyframe.translation, keyframe.translation, interpolationFactor);
            break;
        }
    }
    for (unsigned int i = looped ? 0 : 1; i < boneAnim->scalings.size(); i++) {
        const auto& keyframe = boneAnim->scalings[i];
        if (keyframe.timestamp > time) { // then this is the next keyframe we're going to reach.
            const auto& prevKeyframe = boneAnim->scalings[(i == 0 ? boneAnim->scalings.size() : i) - 1];

            float interpolationFactor = (time - prevKeyframe.timestamp) / (keyframe.timestamp - prevKeyframe.timestamp);
            interpolatedScl = glm::mix(prevKeyframe.scale, keyframe.scale, interpolationFactor);
            break;
        }
    }
    for (unsigned int i = looped ? 0 : 1; i < boneAnim->rotations.size(); i++) {
        const auto& keyframe = boneAnim->rotations[i];
        if (keyframe.timestamp > time) { // then this is the next keyframe we're going to reach.
            const auto& prevKeyframe = boneAnim->rotations[(i == 0 ? boneAnim->rotations.size() : i) - 1];

            float interpolationFactor = (time - prevKeyframe.timestamp) / (keyframe.timestamp - prevKeyframe.timestamp);
            interpolatedRot = glm::normalize(glm::slerp(prevKeyframe.rotation, keyframe.rotation, interpolationFactor)); // TODO: normalizing necessary?
            break;
        }
    }

    auto localMat = glm::translate(glm::identity<glm::mat4x4>(), interpolatedPos)
        * glm::toMat4(interpolatedRot)
        * glm::scale(glm::identity<glm::mat4x4>(), interpolatedScl);

    return localMat;
    
    
}

Skeleton::Skeleton(unsigned nBones) {
    boneTransforms = new glm::mat4x4[nBones];
}

Skeleton::~Skeleton() {
	delete[] boneTransforms;
}

#pragma once
#include "glm/vec3.hpp"
#include <algorithm>

struct AABB {
    glm::dvec3 min;
    glm::dvec3 max;

    inline AABB(glm::dvec3 minPoint = {}, glm::dvec3 maxPoint = {}) {
        min = minPoint;
        max = maxPoint;
    }

    // makes this aabb expand to contain other
    inline void Grow(const AABB& other) {
        min.x = std::min(min.x, other.min.x);
        min.y = std::min(min.y, other.min.y);
        min.z = std::min(min.z, other.min.z);
        max.x = std::max(max.x, other.max.x);
        max.y = std::max(max.y, other.max.y);
        max.z = std::max(max.z, other.max.z);
    }

    // Checks if this AABB fully envelopes the other AABB 
    inline bool TestEnvelopes(const AABB& other) const {
        return (min.x <= other.min.x && min.y <= other.min.y && min.z <= other.min.z && max.x >= other.max.x && max.y >= other.max.y && max.z >= other.max.z);
    }

    // Checks if this AABB envelopes the given point
    inline bool TestEnvelopes(const glm::dvec3& other) const {
        return (min.x <= other.x && min.y <= other.y && min.z <= other.z && max.x >= other.x && max.y >= other.y && max.z >= other.z);
    }

    // returns true if they touching
    inline bool TestIntersection(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) && (min.y <= other.max.y && max.y >= other.min.y) && (min.z <= other.max.z && max.z >= other.min.z);
    }

    // returns true if they touching
    // uses https://tavianator.com/2011/ray_box.html 
    inline bool TestIntersection(const glm::dvec3& origin, const glm::dvec3& direction_inverse) const {
        //std::printf("Testing AABB going from %f %f %f to %f %f %f\n.", min.x, min.y, min.z, max.x, max.y, max.z);

        double t1 = (min[0] - origin[0]) * direction_inverse[0];
        double t2 = (max[0] - origin[0]) * direction_inverse[0];

        double tmin = std::min(t1, t2);
        double tmax = std::max(t1, t2);

        for (int i = 1; i < 3; ++i) {
            t1 = (min[i] - origin[i]) * direction_inverse[i];
            t2 = (max[i] - origin[i]) * direction_inverse[i];

            tmin = std::min(std::max(t1, tmin), std::max(t2, tmin));
            tmax = std::max(std::min(t1, tmax), std::min(t2, tmax));
        }

        return tmax > std::max(tmin, 0.0);
    }

    // returns average of min and max
    inline glm::dvec3 Center() const {
        return (min + max) * 0.5;
    }

    // returns AABB's volume
    inline double Volume() const {
        auto m = max - min;
        return m.x * m.y * m.z;
    }
};
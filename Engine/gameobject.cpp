#include "gameobject.hpp"
#include "rendergroup.hpp"
#include <glm/gtx/quaternion.hpp>
#include "static_meshpool.hpp"
#include "mesh.hpp"
#include "assert.hpp"

Gameobject* Gameobject::New(const GameobjectCreateParams& params)
{
    return MemoryPool<Gameobject, const GameobjectCreateParams&>::Get().New(params);
}

std::shared_ptr<Mesh> Gameobject::GetMesh() {
    if (!meshpool) return nullptr;
    else {
        return renderGroup->GetMesh(this);
    }
}

void Gameobject::SetBoneTransform(unsigned index, glm::mat4x4 transform) {
    auto& skeleton = skeletons.at(this);
    skeleton.boneTransforms[index] = transform;
    skeleton.state = SkeletonState::Dirty;
}

std::optional<Collision> Gameobject::TestCollision(Gameobject* other)
{
    Assert(collider && other->collider);
    if (!collider->aabb.TestIntersection(other->collider->aabb)) return std::nullopt;
    else return NarrowphaseCollisionDetection(this, other);
    
}

#pragma warning(disable : 26495)
Gameobject::Gameobject(const GameobjectCreateParams& params) {
    live = true;
    normalMatDirty = true;
    components = nullptr; // will be assigned on first creation of component

    friction = 0.5;
    elasticity = 0.5;

    position = { 0, 0, 0 };
    rotation = glm::identity<glm::quat>();
    scale = { 1, 1, 1 };
    rotSclDirty = true;

    if (params.mesh) {
        RenderGroup::FindRendergroupForGameobject(*this, params);
        
        if (params.mesh->bones.size() > 0) {
			skeletons.emplace(std::make_pair(this, Skeleton(params.mesh->bones.size())));
            for (unsigned i = 0; i < params.mesh->bones.size(); i++) {
                skeletons.at(this).boneTransforms[i] = glm::identity<glm::mat4x4>();
            }
            skeletons.at(this).state = SkeletonState::Dirty;
        }
    }
    else {
        meshpool = nullptr;
    }

    if (params.physicsMesh) {
        collider = std::make_unique<Collider>(params.physicsMesh, this);
    }

    onGameobjectCreated.FireNow(this);
}

Gameobject::~Gameobject() {
    onGameobjectDestroyed.FireNow(this);

    if (meshpool) renderGroup->RemoveGameobject(*this);
    if (skeletons.contains(this)) skeletons.erase(this);
    live = false;
}

void Gameobject::operator delete(void* obj) {
    MemoryPool<Gameobject, const GameobjectCreateParams&>::Get().Destroy(reinterpret_cast<Gameobject*>(obj));
}

const glm::dvec3& Gameobject::Position() const {
    return position;
}

const glm::vec3& Gameobject::Scale() const {
    return scale;
}

const glm::quat& Gameobject::Rotation() const {
    return rotation;
}

void Gameobject::SetPosition(const glm::dvec3& p) {
    position = p;
    if (collider) {
        collider->UpdateAABB();
        GameobjectSAS().UpdatePosition(collider.get());
    }
}

void Gameobject::SetScale(const glm::vec3& s) {
    scale = s;
    rotSclDirty = true;
    normalMatDirty = true;

    if (collider) {
        collider->UpdateAABB();
        GameobjectSAS().UpdatePosition(collider.get());
    }
}

void Gameobject::SetRotation(const glm::quat& r) {
    rotation = r;
    rotSclDirty = true;
    normalMatDirty = true;

    if (collider) {
        collider->UpdateAABB();
        GameobjectSAS().UpdatePosition(collider.get());
    }
}

void Gameobject::SetInstanceAttribute(const VertexAttribute& attrib, VertexScalar* value) {
    meshpool->SetInstancedVertexAttribute(drawInstanceIndex, attrib, value);
}

void Gameobject::SetInstanceAttribute(const VertexAttribute& attrib, glm::vec4 value) {
    Assert(attrib.nComponents == 4 && attrib.type == VertexScalarType::f32);
    SetInstanceAttribute(attrib, reinterpret_cast<VertexScalar*>(&value));
}

void Gameobject::SetInstanceAttribute(const VertexAttribute& attrib, float value) {
    Assert(attrib.nComponents == 1 && attrib.type == VertexScalarType::f32);
    SetInstanceAttribute(attrib, reinterpret_cast<VertexScalar*>(&value));
}

const glm::mat3x3& Gameobject::GetRotSclMatrix() {
    if (rotSclDirty) {
        rotSclMatrix = glm::identity<glm::mat3x3>();
        rotSclMatrix[0][0] = scale.x;
        rotSclMatrix[1][1] = scale.y;
        rotSclMatrix[2][2] = scale.z;
        rotSclMatrix = glm::toMat3(rotation) * rotSclMatrix;
        rotSclDirty = false;
    }
    return rotSclMatrix;
}

bool Gameobject::Live() {
    return live;
}

Collider* const Gameobject::GetCollider() const {
    return collider.get();
}

glm::vec3 Gameobject::ObjectNormalToWorld(glm::vec3 normal) {
    const auto& rotscl = GetRotSclMatrix();
    return glm::normalize(
        normal.x / Scale().x * rotscl[0] +
        normal.y / Scale().y * rotscl[1] +
        normal.z / Scale().z * rotscl[2]
    );
}

glm::vec3 Gameobject::WorldNormalToObject(glm::vec3 worldNormal)
{
    Assert(false);
    return glm::vec3(0);
}

std::pair<float, float> Physobject::GetInverseReducedMass(glm::vec3 torqueAxis) {
    float inertiaAroundTorqueAxis = 0;
    if (glm::length2(torqueAxis) != 0) {
        auto localAxis = glm::inverse(Rotation()) * glm::normalize(torqueAxis);
        inertiaAroundTorqueAxis = glm::dot(localAxis, inverseInertiaTensor * localAxis);
    }
    float reducedInverseMass1 = inverseMass + glm::length2(torqueAxis) * inertiaAroundTorqueAxis;
    return std::make_pair(reducedInverseMass1, inertiaAroundTorqueAxis);
}

Physobject::Physobject(const PhysobjectCreateParams& params): Gameobject(params) {
    velocity = { 0, 0, 0 };
    rotVelocity = { 0, 0, 0 };
    inverseMass = 1;
    UpdateMass();
}

void Physobject::UpdateMass() {
    float mass = 1 / inverseMass;
    if (collider) {
		auto inertiaTensor = collider->physicsMesh->GetMomentOfInertia(Scale(), mass);
        if (inertiaTensor[1][0] == 0 && inertiaTensor[1][2] == 0 && inertiaTensor[0][1] == 0 && inertiaTensor[2][1] == 0 && inertiaTensor[2][0] == 0 && inertiaTensor[0][2] == 0) {
            // then it's a diagonal matrix; invert diagonal elements. this is neccesary so that if someone's inertia tensor contains infinities (to disable rotation) we don't get NANs
			inverseInertiaTensor = {
				1 / inertiaTensor[0][0], 0, 0,
				0, 1 / inertiaTensor[1][1], 0,
				0, 0, 1 / inertiaTensor[2][2]
			};
        }
        else {
            inverseInertiaTensor = glm::inverse(inertiaTensor);
        }
    }
    else {
        inverseInertiaTensor = { // todo: questionable physical accuracy
            inverseMass, 0, 0,
            0, inverseMass, 0,
            0, 0, inverseMass
        };
    }
}

Physobject* Physobject::New(const PhysobjectCreateParams& params)
{
    return MemoryPool<Physobject, const PhysobjectCreateParams&>::Get().New(params);
}

void Physobject::operator delete(void* obj) {
    MemoryPool<Physobject, const PhysobjectCreateParams&>::Get().Destroy(reinterpret_cast<Physobject*>(obj));
}

Physobject::~Physobject() {

}

void Physobject::SetScale(const glm::vec3& scl) {
    Gameobject::SetScale(scl);
    UpdateMass();
}

void Physobject::SetMass(float mass) {
    inverseMass = 1 / mass;
    UpdateMass();
}

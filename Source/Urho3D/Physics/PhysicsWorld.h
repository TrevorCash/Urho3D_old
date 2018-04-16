//
// Copyright (c) 2008-2018 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Container/HashSet.h"
#include "../IO/VectorBuffer.h"
#include "../Math/BoundingBox.h"
#include "../Math/Sphere.h"
#include "../Math/Vector3.h"
#include "../Scene/Component.h"


namespace Urho3D
{

class CollisionShape;
class Deserializer;
class Constraint;
class Model;
class Node;
class Ray;
class RigidBody;
class Scene;
class Serializer;
class XMLElement;

struct CollisionGeometryData;

/// Physics raycast hit.
struct URHO3D_API PhysicsRaycastResult
{
    /// Construct with defaults.
    PhysicsRaycastResult() :
        body_(nullptr)
    {
    }

    /// Test for inequality, added to prevent GCC from complaining.
    bool operator !=(const PhysicsRaycastResult& rhs) const
    {
        return position_ != rhs.position_ || normal_ != rhs.normal_ || distance_ != rhs.distance_ || body_ != rhs.body_;
    }

    /// Hit worldspace position.
    Vector3 position_;
    /// Hit worldspace normal.
    Vector3 normal_;
    /// Hit distance from ray origin.
    float distance_;
    /// Hit fraction.
    float hitFraction_;
    /// Rigid body that was hit.
    RigidBody* body_;
};

static const float DEFAULT_MAX_NETWORK_ANGULAR_VELOCITY = 100.0f;

/// Cache of collision geometry data.
using CollisionGeometryDataCache = HashMap<Pair<Model*, unsigned>, SharedPtr<CollisionGeometryData> >;

/// Physics simulation world component. Should be added only to the root scene node.
class URHO3D_API PhysicsWorld : public Component
{
    URHO3D_OBJECT(PhysicsWorld, Component);


public:
    /// Construct.
    explicit PhysicsWorld(Context* context);
    /// Destruct.
    ~PhysicsWorld() override;
    /// Register object factory.
    static void RegisterObject(Context* context);


    /// Visualize the component as debug geometry.
    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Step the simulation forward.
    void Update(float timeStep);


    /// Set maximum angular velocity for network replication.
    void SetMaxNetworkAngularVelocity(float velocity);
    /// Perform a physics world raycast and return all hits.
    void Raycast
        (PODVector<PhysicsRaycastResult>& result, const Ray& ray, float maxDistance, unsigned collisionMask = M_MAX_UNSIGNED);
    /// Perform a physics world raycast and return the closest hit.
    void RaycastSingle(PhysicsRaycastResult& result, const Ray& ray, float maxDistance, unsigned collisionMask = M_MAX_UNSIGNED);
    /// Perform a physics world segmented raycast and return the closest hit. Useful for big scenes with many bodies.
    void RaycastSingleSegmented(PhysicsRaycastResult& result, const Ray& ray, float maxDistance, float segmentDistance, unsigned collisionMask = M_MAX_UNSIGNED);
    /// Perform a physics world swept sphere test and return the closest hit.
    void SphereCast
        (PhysicsRaycastResult& result, const Ray& ray, float radius, float maxDistance, unsigned collisionMask = M_MAX_UNSIGNED);
    /// Perform a physics world swept convex test using a user-supplied collision shape and return the first hit.
    void ConvexCast(PhysicsRaycastResult& result, CollisionShape* shape, const Vector3& startPos, const Quaternion& startRot,
        const Vector3& endPos, const Quaternion& endRot, unsigned collisionMask = M_MAX_UNSIGNED);
    /// Invalidate cached collision geometry for a model.
    void RemoveCachedGeometry(Model* model);
    /// Return rigid bodies by a sphere query.
    void GetRigidBodies(PODVector<RigidBody*>& result, const Sphere& sphere, unsigned collisionMask = M_MAX_UNSIGNED);
    /// Return rigid bodies by a box query.
    void GetRigidBodies(PODVector<RigidBody*>& result, const BoundingBox& box, unsigned collisionMask = M_MAX_UNSIGNED);
    /// Return rigid bodies by contact test with the specified body. It needs to be active to return all contacts reliably.
    void GetRigidBodies(PODVector<RigidBody*>& result, const RigidBody* body);
    /// Return rigid bodies that have been in collision with the specified body on the last simulation step. Only returns collisions that were sent as events (depends on collision event mode) and excludes e.g. static-static collisions.
    void GetCollidingBodies(PODVector<RigidBody*>& result, const RigidBody* body);

    /// Return gravity.
    Vector3 GetGravity() const;

 

protected:
    /// Handle scene being assigned.
    void OnSceneSet(Scene* scene) override;

private:
    /// Handle the scene subsystem update event, step simulation here.
    void HandleSceneSubsystemUpdate(StringHash eventType, VariantMap& eventData);
    /// Trigger update before each physics simulation step.
    void PreStep(float timeStep);
    /// Trigger update after each physics simulation step.
    void PostStep(float timeStep);
    /// Send accumulated collision events.
    void SendCollisionEvents();


};

/// Register Physics library objects.
void URHO3D_API RegisterPhysicsLibrary(Context* context);

}

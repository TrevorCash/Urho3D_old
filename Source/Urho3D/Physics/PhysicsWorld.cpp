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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/Mutex.h"
#include "../Core/Profiler.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/Model.h"
#include "../IO/Log.h"
#include "../Math/Ray.h"
#include "../Physics/CollisionShape.h"
#include "../Physics/Constraint.h"
#include "../Physics/PhysicsEvents.h"
#include "../Physics/PhysicsUtils.h"
#include "../Physics/PhysicsWorld.h"
#include "../Physics/RaycastVehicle.h"
#include "../Physics/RigidBody.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"



namespace Urho3D
{

const char* PHYSICS_CATEGORY = "Physics";
extern const char* SUBSYSTEM_CATEGORY;

static const Vector3 DEFAULT_GRAVITY = Vector3(0.0f, -9.81f, 0.0f);


static bool CompareRaycastResults(const PhysicsRaycastResult& lhs, const PhysicsRaycastResult& rhs)
{
    return lhs.distance_ < rhs.distance_;
}




PhysicsWorld::PhysicsWorld(Context* context) : Component(context)
{
   
}

PhysicsWorld::~PhysicsWorld()
{
}

void PhysicsWorld::RegisterObject(Context* context)
{
    context->RegisterFactory<PhysicsWorld>(SUBSYSTEM_CATEGORY);

}

void PhysicsWorld::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug)
    {
        URHO3D_PROFILE(PhysicsDrawDebug);

    }
}


void PhysicsWorld::Update(float timeStep)
{
    URHO3D_PROFILE(UpdatePhysics);

    //float internalTimeStep = 1.0f / fps_;
    //int maxSubSteps = (int)(timeStep * fps_) + 1;
    //if (maxSubSteps_ < 0)
    //{
    //    internalTimeStep = timeStep;
    //    maxSubSteps = 1;
    //}
    //else if (maxSubSteps_ > 0)
    //    maxSubSteps = Min(maxSubSteps, maxSubSteps_);

    //delayedWorldTransforms_.Clear();
    //simulating_ = true;

    //if (interpolation_)
    //    world_->stepSimulation(timeStep, maxSubSteps, internalTimeStep);
    //else
    //{
    //    timeAcc_ += timeStep;
    //    while (timeAcc_ >= internalTimeStep && maxSubSteps > 0)
    //    {
    //        world_->stepSimulation(internalTimeStep, 0, internalTimeStep);
    //        timeAcc_ -= internalTimeStep;
    //        --maxSubSteps;
    //    }
    //}

    //simulating_ = false;

    //// Apply delayed (parented) world transforms now
    //while (!delayedWorldTransforms_.Empty())
    //{
    //    for (HashMap<RigidBody*, DelayedWorldTransform>::Iterator i = delayedWorldTransforms_.Begin();
    //         i != delayedWorldTransforms_.End();)
    //    {
    //        const DelayedWorldTransform& transform = i->second_;

    //        // If parent's transform has already been assigned, can proceed
    //        if (!delayedWorldTransforms_.Contains(transform.parentRigidBody_))
    //        {
    //            transform.rigidBody_->ApplyWorldTransform(transform.worldPosition_, transform.worldRotation_);
    //            i = delayedWorldTransforms_.Erase(i);
    //        }
    //        else
    //            ++i;
    //    }
    //}
}


void PhysicsWorld::OnSceneSet(Scene* scene)
{
    // Subscribe to the scene subsystem update, which will trigger the physics simulation step
    if (GetScene())
    {
        SubscribeToEvent(GetScene(), E_SCENESUBSYSTEMUPDATE, URHO3D_HANDLER(PhysicsWorld, HandleSceneSubsystemUpdate));
    }
    else
        UnsubscribeFromEvent(E_SCENESUBSYSTEMUPDATE);
}

void PhysicsWorld::HandleSceneSubsystemUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace SceneSubsystemUpdate;
    Update(eventData[P_TIMESTEP].GetFloat());
}

void PhysicsWorld::PreStep(float timeStep)
{
    // Send pre-step event
    using namespace PhysicsPreStep;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_WORLD] = this;
    eventData[P_TIMESTEP] = timeStep;
    SendEvent(E_PHYSICSPRESTEP, eventData);

    // Start profiling block for the actual simulation step
    URHO3D_PROFILE_NONSCOPED(PhysicsStepSimulation);
}

void PhysicsWorld::PostStep(float timeStep)
{
    URHO3D_PROFILE_END();

    SendCollisionEvents();

    // Send post-step event
    using namespace PhysicsPostStep;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_WORLD] = this;
    eventData[P_TIMESTEP] = timeStep;
    SendEvent(E_PHYSICSPOSTSTEP, eventData);
}

void PhysicsWorld::SendCollisionEvents()
{
    URHO3D_PROFILE(SendCollisionEvents);

    //currentCollisions_.Clear();
    //physicsCollisionData_.Clear();
    //nodeCollisionData_.Clear();

    //int numManifolds = collisionDispatcher_->getNumManifolds();

    //if (numManifolds)
    //{
    //    physicsCollisionData_[PhysicsCollision::P_WORLD] = this;

    //    for (int i = 0; i < numManifolds; ++i)
    //    {
    //        btPersistentManifold* contactManifold = collisionDispatcher_->getManifoldByIndexInternal(i);
    //        // First check that there are actual contacts, as the manifold exists also when objects are close but not touching
    //        if (!contactManifold->getNumContacts())
    //            continue;

    //        const btCollisionObject* objectA = contactManifold->getBody0();
    //        const btCollisionObject* objectB = contactManifold->getBody1();

    //        auto* bodyA = static_cast<RigidBody*>(objectA->getUserPointer());
    //        auto* bodyB = static_cast<RigidBody*>(objectB->getUserPointer());
    //        // If it's not a rigidbody, maybe a ghost object
    //        if (!bodyA || !bodyB)
    //            continue;

    //        // Skip collision event signaling if both objects are static, or if collision event mode does not match
    //        if (bodyA->GetMass() == 0.0f && bodyB->GetMass() == 0.0f)
    //            continue;
    //        if (bodyA->GetCollisionEventMode() == COLLISION_NEVER || bodyB->GetCollisionEventMode() == COLLISION_NEVER)
    //            continue;
    //        if (bodyA->GetCollisionEventMode() == COLLISION_ACTIVE && bodyB->GetCollisionEventMode() == COLLISION_ACTIVE &&
    //            !bodyA->IsActive() && !bodyB->IsActive())
    //            continue;

    //        WeakPtr<RigidBody> bodyWeakA(bodyA);
    //        WeakPtr<RigidBody> bodyWeakB(bodyB);

    //        // First only store the collision pair as weak pointers and the manifold pointer, so user code can safely destroy
    //        // objects during collision event handling
    //        Pair<WeakPtr<RigidBody>, WeakPtr<RigidBody> > bodyPair;
    //        if (bodyA < bodyB)
    //        {
    //            bodyPair = MakePair(bodyWeakA, bodyWeakB);
    //            currentCollisions_[bodyPair].manifold_ = contactManifold;
    //        }
    //        else
    //        {
    //            bodyPair = MakePair(bodyWeakB, bodyWeakA);
    //            currentCollisions_[bodyPair].flippedManifold_ = contactManifold;
    //        }
    //    }

    //    for (HashMap<Pair<WeakPtr<RigidBody>, WeakPtr<RigidBody> >, ManifoldPair>::Iterator i = currentCollisions_.Begin();
    //         i != currentCollisions_.End(); ++i)
    //    {
    //        RigidBody* bodyA = i->first_.first_;
    //        RigidBody* bodyB = i->first_.second_;
    //        if (!bodyA || !bodyB)
    //            continue;

    //        Node* nodeA = bodyA->GetNode();
    //        Node* nodeB = bodyB->GetNode();
    //        WeakPtr<Node> nodeWeakA(nodeA);
    //        WeakPtr<Node> nodeWeakB(nodeB);

    //        bool trigger = bodyA->IsTrigger() || bodyB->IsTrigger();
    //        bool newCollision = !previousCollisions_.Contains(i->first_);

    //        physicsCollisionData_[PhysicsCollision::P_NODEA] = nodeA;
    //        physicsCollisionData_[PhysicsCollision::P_NODEB] = nodeB;
    //        physicsCollisionData_[PhysicsCollision::P_BODYA] = bodyA;
    //        physicsCollisionData_[PhysicsCollision::P_BODYB] = bodyB;
    //        physicsCollisionData_[PhysicsCollision::P_TRIGGER] = trigger;

    //        contacts_.Clear();

    //        // "Pointers not flipped"-manifold, send unmodified normals
    //        btPersistentManifold* contactManifold = i->second_.manifold_;
    //        if (contactManifold)
    //        {
    //            for (int j = 0; j < contactManifold->getNumContacts(); ++j)
    //            {
    //                btManifoldPoint& point = contactManifold->getContactPoint(j);
    //                contacts_.WriteVector3(ToVector3(point.m_positionWorldOnB));
    //                contacts_.WriteVector3(ToVector3(point.m_normalWorldOnB));
    //                contacts_.WriteFloat(point.m_distance1);
    //                contacts_.WriteFloat(point.m_appliedImpulse);
    //            }
    //        }
    //        // "Pointers flipped"-manifold, flip normals also
    //        contactManifold = i->second_.flippedManifold_;
    //        if (contactManifold)
    //        {
    //            for (int j = 0; j < contactManifold->getNumContacts(); ++j)
    //            {
    //                btManifoldPoint& point = contactManifold->getContactPoint(j);
    //                contacts_.WriteVector3(ToVector3(point.m_positionWorldOnB));
    //                contacts_.WriteVector3(-ToVector3(point.m_normalWorldOnB));
    //                contacts_.WriteFloat(point.m_distance1);
    //                contacts_.WriteFloat(point.m_appliedImpulse);
    //            }
    //        }

    //        physicsCollisionData_[PhysicsCollision::P_CONTACTS] = contacts_.GetBuffer();

    //        // Send separate collision start event if collision is new
    //        if (newCollision)
    //        {
    //            SendEvent(E_PHYSICSCOLLISIONSTART, physicsCollisionData_);
    //            // Skip rest of processing if either of the nodes or bodies is removed as a response to the event
    //            if (!nodeWeakA || !nodeWeakB || !i->first_.first_ || !i->first_.second_)
    //                continue;
    //        }

    //        // Then send the ongoing collision event
    //        SendEvent(E_PHYSICSCOLLISION, physicsCollisionData_);
    //        if (!nodeWeakA || !nodeWeakB || !i->first_.first_ || !i->first_.second_)
    //            continue;

    //        nodeCollisionData_[NodeCollision::P_BODY] = bodyA;
    //        nodeCollisionData_[NodeCollision::P_OTHERNODE] = nodeB;
    //        nodeCollisionData_[NodeCollision::P_OTHERBODY] = bodyB;
    //        nodeCollisionData_[NodeCollision::P_TRIGGER] = trigger;
    //        nodeCollisionData_[NodeCollision::P_CONTACTS] = contacts_.GetBuffer();

    //        if (newCollision)
    //        {
    //            nodeA->SendEvent(E_NODECOLLISIONSTART, nodeCollisionData_);
    //            if (!nodeWeakA || !nodeWeakB || !i->first_.first_ || !i->first_.second_)
    //                continue;
    //        }

    //        nodeA->SendEvent(E_NODECOLLISION, nodeCollisionData_);
    //        if (!nodeWeakA || !nodeWeakB || !i->first_.first_ || !i->first_.second_)
    //            continue;

    //        // Flip perspective to body B
    //        contacts_.Clear();
    //        contactManifold = i->second_.manifold_;
    //        if (contactManifold)
    //        {
    //            for (int j = 0; j < contactManifold->getNumContacts(); ++j)
    //            {
    //                btManifoldPoint& point = contactManifold->getContactPoint(j);
    //                contacts_.WriteVector3(ToVector3(point.m_positionWorldOnB));
    //                contacts_.WriteVector3(-ToVector3(point.m_normalWorldOnB));
    //                contacts_.WriteFloat(point.m_distance1);
    //                contacts_.WriteFloat(point.m_appliedImpulse);
    //            }
    //        }
    //        contactManifold = i->second_.flippedManifold_;
    //        if (contactManifold)
    //        {
    //            for (int j = 0; j < contactManifold->getNumContacts(); ++j)
    //            {
    //                btManifoldPoint& point = contactManifold->getContactPoint(j);
    //                contacts_.WriteVector3(ToVector3(point.m_positionWorldOnB));
    //                contacts_.WriteVector3(ToVector3(point.m_normalWorldOnB));
    //                contacts_.WriteFloat(point.m_distance1);
    //                contacts_.WriteFloat(point.m_appliedImpulse);
    //            }
    //        }

    //        nodeCollisionData_[NodeCollision::P_BODY] = bodyB;
    //        nodeCollisionData_[NodeCollision::P_OTHERNODE] = nodeA;
    //        nodeCollisionData_[NodeCollision::P_OTHERBODY] = bodyA;
    //        nodeCollisionData_[NodeCollision::P_CONTACTS] = contacts_.GetBuffer();

    //        if (newCollision)
    //        {
    //            nodeB->SendEvent(E_NODECOLLISIONSTART, nodeCollisionData_);
    //            if (!nodeWeakA || !nodeWeakB || !i->first_.first_ || !i->first_.second_)
    //                continue;
    //        }

    //        nodeB->SendEvent(E_NODECOLLISION, nodeCollisionData_);
    //    }
    //}

    //// Send collision end events as applicable
    //{
    //    physicsCollisionData_[PhysicsCollisionEnd::P_WORLD] = this;

    //    for (HashMap<Pair<WeakPtr<RigidBody>, WeakPtr<RigidBody> >, ManifoldPair>::Iterator
    //             i = previousCollisions_.Begin(); i != previousCollisions_.End(); ++i)
    //    {
    //        if (!currentCollisions_.Contains(i->first_))
    //        {
    //            RigidBody* bodyA = i->first_.first_;
    //            RigidBody* bodyB = i->first_.second_;
    //            if (!bodyA || !bodyB)
    //                continue;

    //            bool trigger = bodyA->IsTrigger() || bodyB->IsTrigger();

    //            // Skip collision event signaling if both objects are static, or if collision event mode does not match
    //            if (bodyA->GetMass() == 0.0f && bodyB->GetMass() == 0.0f)
    //                continue;
    //            if (bodyA->GetCollisionEventMode() == COLLISION_NEVER || bodyB->GetCollisionEventMode() == COLLISION_NEVER)
    //                continue;
    //            if (bodyA->GetCollisionEventMode() == COLLISION_ACTIVE && bodyB->GetCollisionEventMode() == COLLISION_ACTIVE &&
    //                !bodyA->IsActive() && !bodyB->IsActive())
    //                continue;

    //            Node* nodeA = bodyA->GetNode();
    //            Node* nodeB = bodyB->GetNode();
    //            WeakPtr<Node> nodeWeakA(nodeA);
    //            WeakPtr<Node> nodeWeakB(nodeB);

    //            physicsCollisionData_[PhysicsCollisionEnd::P_BODYA] = bodyA;
    //            physicsCollisionData_[PhysicsCollisionEnd::P_BODYB] = bodyB;
    //            physicsCollisionData_[PhysicsCollisionEnd::P_NODEA] = nodeA;
    //            physicsCollisionData_[PhysicsCollisionEnd::P_NODEB] = nodeB;
    //            physicsCollisionData_[PhysicsCollisionEnd::P_TRIGGER] = trigger;

    //            SendEvent(E_PHYSICSCOLLISIONEND, physicsCollisionData_);
    //            // Skip rest of processing if either of the nodes or bodies is removed as a response to the event
    //            if (!nodeWeakA || !nodeWeakB || !i->first_.first_ || !i->first_.second_)
    //                continue;

    //            nodeCollisionData_[NodeCollisionEnd::P_BODY] = bodyA;
    //            nodeCollisionData_[NodeCollisionEnd::P_OTHERNODE] = nodeB;
    //            nodeCollisionData_[NodeCollisionEnd::P_OTHERBODY] = bodyB;
    //            nodeCollisionData_[NodeCollisionEnd::P_TRIGGER] = trigger;

    //            nodeA->SendEvent(E_NODECOLLISIONEND, nodeCollisionData_);
    //            if (!nodeWeakA || !nodeWeakB || !i->first_.first_ || !i->first_.second_)
    //                continue;

    //            nodeCollisionData_[NodeCollisionEnd::P_BODY] = bodyB;
    //            nodeCollisionData_[NodeCollisionEnd::P_OTHERNODE] = nodeA;
    //            nodeCollisionData_[NodeCollisionEnd::P_OTHERBODY] = bodyA;

    //            nodeB->SendEvent(E_NODECOLLISIONEND, nodeCollisionData_);
    //        }
    //    }
    //}

    //previousCollisions_ = currentCollisions_;
}

void RegisterPhysicsLibrary(Context* context)
{
    CollisionShape::RegisterObject(context);
    RigidBody::RegisterObject(context);
    Constraint::RegisterObject(context);
    PhysicsWorld::RegisterObject(context);
    RaycastVehicle::RegisterObject(context);
}

}

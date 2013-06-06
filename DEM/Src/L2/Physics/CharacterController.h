#pragma once
#ifndef __DEM_L2_CHARACTER_CTLR_H__
#define __DEM_L2_CHARACTER_CTLR_H__

#include <Core/RefCounted.h>

// Character controller is used to drive characters. It gets desired velocities and other commands
// as input and calculates final transform, taking different character properties into account.
// This is a dynamic implementation, that uses a rigid body and works from physics to scene.
// Maybe later we will implement a kinematic one too. Kinematic controller is more controllable
// and is bound to the navmesh, but all the collision detection and response must be processed manually.

namespace Data
{
	class CParams;
}

namespace Physics
{
typedef Ptr<class CRigidBody> PRigidBody;
class CPhysicsWorld;

class CCharacterController: public Core::CRefCounted
{
protected:

	//!!!Slide down from where can't climb up and don't slide where can climb!
	//Slide along vertical obstacles, don't bounce

	float		Radius;
	float		Height;
	float		Hover;
	float		MaxSlopeAngle;	// In radians //???recalc to cos?
	float		MaxClimb; //???recalc to hover?
	float		MaxJump; //???!!!recalc to max jump impulse?
	float		Softness;		// Allowed penetration depth //???!!!recalc to bullet collision margin?!

	//???need here? or Bullet can do this? if zero or less, don't use
	float		MaxAcceleration;

	PRigidBody	Body;

	vector3		ReqLinVel;
	float		ReqAngVel;

public:

	~CCharacterController() { Term(); }

	bool		Init(const Data::CParams& Desc);
	void		Term();
	bool		AttachToLevel(CPhysicsWorld& World);
	void		RemoveFromLevel();
	bool		IsAttachedToLevel() const;

	void		Update();

	void		RequestLinearVelocity(const vector3& Velocity);
	void		RequestAngularVelocity(float Velocity);

	CRigidBody*	GetBody() const { return Body.GetUnsafe(); }

	// SetTransform - teleport
	// GetTransform
	// GetLinearVelocity
	// GetAngularVelocity

	//!!!collision callbacks/events! fire from global or from here?

	//IsOnTheGround / GetGroundInfo / IsFalling
	//Jump //???Fall if touched the ceiling?
	//???StartCrouch, StopCrouch (if can't, now will remember request), IsCrouching?
};

typedef Ptr<CCharacterController> PCharacterController;

}

#endif

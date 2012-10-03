#pragma once
#ifndef __DEM_L2_PHYSICS_JOINT_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_JOINT_H__

#include <Physics/RigidBody.h>
#include <Data/Params.h>
#define dSINGLE
#include <ode/ode.h>

// A joint (also known as constraint) connects to rigid bodies. Subclasses
// of joint implement specific joint types.

namespace Physics
{
class CJointAxis;

class CJoint: public Core::CRefCounted
{
	DeclareRTTI;

protected:

	dJointID	ODEJointID;
	PRigidBody	pBody1;
	PRigidBody	pBody2;

	static void	InitAxis(CJointAxis* pAxis, PParams Desc);
	vector4		GetDebugVisualizationColor() const { return vector4(1.0f, 0.0f, 1.0f, 1.0f); }

public:
	
	nString	LinkName;
	int		LinkIndex;

	CJoint(): ODEJointID(NULL) {}
	virtual ~CJoint() = 0;

	virtual void Init(PParams Desc) {}
	virtual void Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm);
	virtual void Detach();
	virtual void UpdateTransform(const matrix44& Tfm) = 0;
	virtual void RenderDebug();

	bool IsAttached() const { return ODEJointID != NULL; }

	void				SetBodies(CRigidBody* pRigidBody1, CRigidBody* pRigidBody2);
	void				SetBody1(CRigidBody* pBody);
	void				SetBody2(CRigidBody* pBody);
	const CRigidBody*	GetBody1() const { return pBody1.get_unsafe(); }
	const CRigidBody*	GetBody2() const { return pBody2.get_unsafe(); }
	bool				IsLinkValid() const { return LinkName.IsValid(); }
	dJointID			GetJointId() const { return ODEJointID; }
};
//---------------------------------------------------------------------

typedef Ptr<CJoint> PJoint;

inline void CJoint::SetBodies(CRigidBody* pRigidBody1, CRigidBody* pRigidBody2)
{
	pBody1 = pRigidBody1;
	pBody2 = pRigidBody2;
}
//---------------------------------------------------------------------

inline void CJoint::SetBody1(CRigidBody* pBody)
{
	pBody1 = pBody;
}
//---------------------------------------------------------------------

inline void CJoint::SetBody2(CRigidBody* pBody)
{
	pBody2 = pBody;
}
//---------------------------------------------------------------------

}

#endif

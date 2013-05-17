#pragma once
#ifndef __DEM_L2_PHYSICS_HINGE_JOINT_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_HINGE_JOINT_H__

#include "Joint.h"
#include "JointAxis.h"

// A hinge joint. See ODE docs for details.

namespace Physics
{

class �HingeJoint: public CJoint
{
	__DeclareClass(�HingeJoint);

public:

	vector3		Anchor;
	CJointAxis	AxisParams;

	�HingeJoint();
	virtual ~�HingeJoint() {}
	
	virtual void Init(PParams Desc);
	virtual void Attach(dWorldID WorldID, dJointGroupID GroupID, const matrix44& ParentTfm);
	virtual void UpdateTransform(const matrix44& Tfm);
	virtual void RenderDebug();
};

}

#endif

#pragma once
#ifndef __DEM_L1_COLLISION_OBJECT_STATIC_H__
#define __DEM_L1_COLLISION_OBJECT_STATIC_H__

#include <Physics/CollisionObj.h>

// Static collision object can't move, transform changes are discrete and manual.
// Use this type of objects to represent static environment.

namespace Physics
{

class CCollisionObjStatic: public CCollisionObj
{
	__DeclareClassNoFactory;

public:

	//!!!need normal flags!
	virtual bool Init(CCollisionShape& CollShape, ushort Group = 0x0001, ushort Mask = 0xffff, const vector3& Offset = vector3::Zero);
	virtual bool AttachToLevel(CPhysicsWorld& World);
	virtual void RemoveFromLevel();
};

typedef Ptr<CCollisionObjStatic> PCollisionObjStatic;

}

#endif

#pragma once
#ifndef __DEM_L1_COLLISION_SHAPE_H__
#define __DEM_L1_COLLISION_SHAPE_H__

#include <Resources/Resource.h>

// Shared collision shape, which can be used by multiple collision objects and rigid bodies

class btCollisionShape;

namespace Physics
{

class CCollisionShape: public Resources::CResource
{
	__DeclareClass(CCollisionShape);

protected:

	btCollisionShape*	pBtShape;

public:

	CCollisionShape(CStrID ID): CResource(ID), pBtShape(NULL) {}
	virtual ~CCollisionShape() { if (IsLoaded()) Unload(); }

	bool				Setup(btCollisionShape* pShape);
	virtual void		Unload();

	btCollisionShape*	GetBtShape() const { return pBtShape; }
};

typedef Ptr<CCollisionShape> PCollisionShape;

}

#endif

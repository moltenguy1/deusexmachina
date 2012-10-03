#include "FilterSet.h"

#include <Physics/Collision/Shape.h>
#include <Physics/Entity.h>
#include <Physics/rigidbody.h>

namespace Physics
{

bool CFilterSet::CheckShape(CShape* pShape) const
{
	n_assert(pShape);
	return CheckMaterialType(pShape->GetMaterialType()) ||
		(pShape->GetEntity() && CheckEntityID(pShape->GetEntity()->GetUniqueID())) ||
		(pShape->GetRigidBody() && CheckRigidBodyID(pShape->GetRigidBody()->GetUniqueID()));
}
//---------------------------------------------------------------------

};

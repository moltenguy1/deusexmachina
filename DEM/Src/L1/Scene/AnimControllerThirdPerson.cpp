#include "AnimControllerThirdPerson.h"

namespace Scene
{

bool CAnimControllerThirdPerson::ApplyTo(Math::CTransformSRT& DestTfm)
{
	if (!Dirty) FAIL;

	DestTfm.Translation = Angles.get_cartesian_z();
	DestTfm.Rotation.set_from_axes(-vector3::AxisZ, -DestTfm.Translation);
	DestTfm.Translation *= Distance;

	Dirty = false;
	OK;
}
//---------------------------------------------------------------------

}

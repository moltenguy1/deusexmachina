#pragma once
#ifndef __DEM_L1_NODE_CTLR_THIRD_PERSON_H__
#define __DEM_L1_NODE_CTLR_THIRD_PERSON_H__

#include <Scene/NodeController.h>
#include <Math/Polar.h>

// Animation controller, that implements logic of third-person camera.
// This controller updates transform only when it has changed, saving
// lots of recalculations. To force update use ForceNextUpdate().
// All angles are in radians.

namespace Scene
{

class CNodeControllerThirdPerson: public CNodeController
{
protected:

	CPolar	Angles;
	float	Distance;
	vector3	COI;			// Center of interest, eye target in parent coordinates

	float	MinVertAngle;
	float	MaxVertAngle;
	float	MinDistance;
	float	MaxDistance;

	bool	Dirty;

public:

	CNodeControllerThirdPerson();

	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm);

	void			SetVerticalAngleLimits(float Min, float Max);
	void			SetDistanceLimits(float Min, float Max);
	void			SetAngles(float Vertical, float Horizontal);
	void			SetDirection(const vector3& Dir);
	void			SetDistance(float Value);
	void			SetCOI(const vector3& NewCOI);
	void			OrbitVertical(float Angle);
	void			OrbitHorizontal(float Angle);
	void			Zoom(float Amount);
	void			Move(const vector3& Translation);
	void			ForceNextUpdate() { Dirty = true; } //???normal dirty-on-Set instead?

	float			GetVerticalAngleMin() const { return MinVertAngle; }
	float			GetVerticalAngleMax() const { return MaxVertAngle; }
	float			GetDistanceMin() const { return MinDistance; }
	float			GetDistanceMax() const { return MaxDistance; }
	const CPolar&	GetAngles() const {return Angles; }
	float			GetDistance() const {return Distance; }
	const vector3&	GetCOI() const { return COI; }
};

typedef Ptr<CNodeControllerThirdPerson> PNodeControllerThirdPerson;

inline CNodeControllerThirdPerson::CNodeControllerThirdPerson():
	Distance(1.f),
	MinVertAngle(0.f),
	MaxVertAngle(PI * 0.5f),
	MinDistance(0.01f),
	MaxDistance(10000.f),
	Dirty(true)
{
	Flags.Set(LocalSpace); // For now, later mb world space + offset + position of target node
	Channels.Set(Chnl_Translation | Chnl_Rotation);
}
//---------------------------------------------------------------------

inline void CNodeControllerThirdPerson::SetVerticalAngleLimits(float Min, float Max)
{
	n_assert(Min <= Max);
	MinVertAngle = Min;
	MaxVertAngle = Max;
	Angles.Theta = n_clamp(Angles.Theta, MinVertAngle, MaxVertAngle);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CNodeControllerThirdPerson::SetDistanceLimits(float Min, float Max)
{
	n_assert(Min <= Max);
	MinDistance = Min;
	MaxDistance = Max;
	Distance = n_clamp(Distance, MinDistance, MaxDistance);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CNodeControllerThirdPerson::SetAngles(float Vertical, float Horizontal)
{
	Angles.Theta = n_clamp(Vertical, MinVertAngle, MaxVertAngle);
	Angles.Phi = Horizontal;
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CNodeControllerThirdPerson::SetDirection(const vector3& Dir)
{
	Angles.Set(Dir); //???or -Dir?
	Angles.Theta = n_clamp(Angles.Theta, MinVertAngle, MaxVertAngle);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CNodeControllerThirdPerson::SetDistance(float Value)
{
	Distance = n_clamp(Value, MinDistance, MaxDistance);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CNodeControllerThirdPerson::SetCOI(const vector3& NewCOI)
{
	if (COI == NewCOI) return;
	COI = NewCOI;
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CNodeControllerThirdPerson::OrbitVertical(float Angle)
{
	if (Angle == 0.f) return;
	Angles.Theta = n_clamp(Angles.Theta + Angle, MinVertAngle, MaxVertAngle);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CNodeControllerThirdPerson::OrbitHorizontal(float Angle)
{
	if (Angle == 0.f) return;
	Angles.Phi += Angle;
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CNodeControllerThirdPerson::Zoom(float Amount)
{
	if (Amount == 0.f) return;
	Distance = n_clamp(Distance + Amount, MinDistance, MaxDistance);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CNodeControllerThirdPerson::Move(const vector3& Translation)
{
	if (Translation == vector3::Zero) return;
	COI += Translation;
	Dirty = true;
}
//---------------------------------------------------------------------

}

#endif

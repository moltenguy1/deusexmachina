#pragma once
#ifndef __DEM_L2_AI_ACTION_FACE_H__
#define __DEM_L2_AI_ACTION_FACE_H__

#include <AI/Behaviour/Action.h>
#include <Data/StringID.h>

// Face action makes actor face specified direction (Actor->FaceDir).
// This direction must be set externally or by derived class.

namespace AI
{

class CActionFace: public CAction
{
	__DeclareClass(CActionFace);

public:

	virtual EExecStatus	Update(CActor* pActor);
	virtual void		Deactivate(CActor* pActor);
};

typedef Ptr<CActionFace> PActionFace;

}

#endif
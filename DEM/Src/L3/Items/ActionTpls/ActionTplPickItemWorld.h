#pragma once
#ifndef __DEM_L2_AI_ACTION_TPL_PICK_ITEM_WORLD_H__
#define __DEM_L2_AI_ACTION_TPL_PICK_ITEM_WORLD_H__

#include <AI/Planning/ActionTpl.h>

// Actor picks item represented as entity in the location to satisfy WSP_HasItem.
// This tpl generates no action, it makes planner plan UseSmartObj with right params instead.

namespace AI
{

class CActionTplPickItemWorld: public CActionTpl
{
	DeclareRTTI;
	DeclareFactory(CActionTplPickItemWorld);

private:

	//!!!only for synchronous env!
	CStrID ItemEntityID;

public:

	virtual void		Init(PParams Params);
	virtual bool		GetPreconditions(CActor* pActor, CWorldState& WS, const CWorldState& WSGoal) const;
	virtual bool		ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal);
	virtual PAction		CreateInstance(const CWorldState& Context) const;
};

RegisterFactory(CActionTplPickItemWorld);

typedef Ptr<CActionTplPickItemWorld> PActionTplPickItemWorld;

}

#endif
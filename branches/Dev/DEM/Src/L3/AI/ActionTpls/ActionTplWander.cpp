#include "ActionTplWander.h"

#include <AI/Actions/ActionWander.h>

#ifdef __WIN32__
	#ifdef GetProp
		#undef GetProp
		#undef SetProp
	#endif
#endif

namespace AI
{
ImplementRTTI(AI::CActionTplWander, AI::CActionTpl);
ImplementFactory(AI::CActionTplWander);

void CActionTplWander::Init(PParams Params)
{
	CActionTpl::Init(Params);
	WSEffects.SetProp(WSP_Action, CStrID("Wander"));
}
//---------------------------------------------------------------------

bool CActionTplWander::ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal)
{
	return !WSGoal.IsPropSet(WSP_UsingSmartObj) || WSGoal.GetProp(WSP_UsingSmartObj) == CStrID::Empty;
}
//---------------------------------------------------------------------

PAction CActionTplWander::CreateInstance(const CWorldState& Context) const
{
	return n_new(CActionWander);
}
//---------------------------------------------------------------------

} //namespace AI
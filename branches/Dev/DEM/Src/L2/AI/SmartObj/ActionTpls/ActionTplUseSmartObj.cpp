#include "ActionTplUseSmartObj.h"

#include <AI/SmartObj/Actions/ActionUseSmartObj.h>
#include <Game/EntityManager.h>
#include <AI/Prop/PropSmartObject.h>

#ifdef __WIN32__
	#ifdef GetProp
		#undef GetProp
		#undef SetProp
	#endif
#endif

namespace AI
{
__ImplementClassNoFactory(AI::CActionTplUseSmartObj, AI::CActionTpl);
__ImplementClass(AI::CActionTplUseSmartObj);

void CActionTplUseSmartObj::Init(PParams Params)
{
	CActionTpl::Init(Params);
	WSEffects.SetProp(WSP_UsingSmartObj, WSP_UsingSmartObj);
	WSEffects.SetProp(WSP_Action, WSP_Action);
}
//---------------------------------------------------------------------

bool CActionTplUseSmartObj::GetPreconditions(CActor* pActor, CWorldState& WS, const CWorldState& WSGoal) const
{
	//???check here is current position valid? or check somewhere else and DepartNode there?
	WS.SetProp(WSP_AtEntityPos, WSP_UsingSmartObj);

	GetSOPreconditions(pActor, WS, WSGoal.GetProp(WSP_UsingSmartObj), WSGoal.GetProp(WSP_Action));

	OK;
}
//---------------------------------------------------------------------

bool CActionTplUseSmartObj::GetSOPreconditions(CActor* pActor, CWorldState& WS, CStrID SOEntityID, CStrID ActionID) const
{
	Game::CEntity* pEntity = EntityMgr->GetEntity(SOEntityID, true);
	if (pEntity)
	{
		CPropSmartObject* pSO = pEntity->GetProperty<CPropSmartObject>();
		n_assert(pSO);

		PSmartObjAction Action = pSO->GetAction(ActionID);
		if (Action.IsValid() && Action->Preconditions.IsValid())
			return Action->Preconditions->FillWorldState(pActor, pSO, WS);
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CActionTplUseSmartObj::ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal)
{
	// Get pSO, Action
	// Check is action valid for planning (special PlannerContextValidator???)
	return WSGoal.IsPropSet(WSP_UsingSmartObj) && WSGoal.GetProp(WSP_UsingSmartObj) != CStrID::Empty;
}
//---------------------------------------------------------------------

PAction CActionTplUseSmartObj::CreateInstance(const CWorldState& Context) const
{
	PActionUseSmartObj Act = n_new(CActionUseSmartObj);
	Act->Init(Context.GetProp(WSP_UsingSmartObj), Context.GetProp(WSP_Action));
	return Act.GetUnsafe();
}
//---------------------------------------------------------------------

} //namespace AI
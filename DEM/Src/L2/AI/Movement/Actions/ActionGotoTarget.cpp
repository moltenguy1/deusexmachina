#include "ActionGotoTarget.h"

#include <AI/PropActorBrain.h>
#include <Game/EntityManager.h>

namespace AI
{
__ImplementClass(AI::CActionGotoTarget, 'AGTG', AI::CActionGoto)

bool CActionGotoTarget::Activate(CActor* pActor)
{
	Game::CEntity* pEnt = EntityMgr->GetEntity(TargetID);
	if (!pEnt) FAIL;
	pActor->GetNavSystem().SetDestPoint(pEnt->GetAttr<matrix44>(CStrID("Transform")).Translation());

	//!!!Get IsDynamic as (BB->WantToFollow && IsTargetMovable)!
	IsDynamic = false;

	OK;
}
//---------------------------------------------------------------------

EExecStatus CActionGotoTarget::Update(CActor* pActor)
{
	if (IsDynamic && !pActor->IsNavSystemIdle())
	{
		Game::CEntity* pEnt = EntityMgr->GetEntity(TargetID);
		if (!pEnt) return Failure;
		pActor->GetNavSystem().SetDestPoint(pEnt->GetAttr<matrix44>(CStrID("Transform")).Translation());
		if (pActor->NavState == AINav_Done) return Success;
	}

	return CActionGoto::Update(pActor);
}
//---------------------------------------------------------------------

}
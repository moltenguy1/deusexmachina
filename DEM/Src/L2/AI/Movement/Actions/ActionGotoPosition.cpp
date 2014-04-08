#include "ActionGotoPosition.h"

#include <AI/PropActorBrain.h>

namespace AI
{
__ImplementClass(AI::CActionGotoPosition, 'AGPS', AI::CActionGoto)

bool CActionGotoPosition::Activate(CActor* pActor)
{
	pActor->GetNavSystem().SetDestPoint(Pos);
	OK;
}
//---------------------------------------------------------------------

}
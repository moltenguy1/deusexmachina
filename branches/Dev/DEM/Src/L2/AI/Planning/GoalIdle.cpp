#include "GoalIdle.h"

#include <Data/StringID.h>

namespace AI
{
__ImplementClass(AI::CGoalIdle, 'GIDL', AI::CGoal);

void CGoalIdle::GetDesiredProps(CWorldState& Dest)
{
	Dest.SetProp(WSP_Action, CStrID("Idle"));
}
//---------------------------------------------------------------------

}
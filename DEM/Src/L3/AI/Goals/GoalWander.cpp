#include "GoalWander.h"

#include <Data/StringID.h>
#include <Core/Factory.h>

namespace AI
{
__ImplementClass(AI::CGoalWander, 'GWDR', AI::CGoal);

void CGoalWander::GetDesiredProps(CWorldState& Dest)
{
	Dest.SetProp(WSP_Action, CStrID("Wander"));
}
//---------------------------------------------------------------------

} //namespace AI
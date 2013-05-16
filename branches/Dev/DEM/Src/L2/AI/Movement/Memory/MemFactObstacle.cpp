#include "MemFactObstacle.h"

#include <AI/Perception/Stimulus.h>

namespace AI
{
__ImplementClass(AI::CMemFactObstacle, 'MFOB', AI::CMemFact);

bool CMemFactObstacle::Match(const CMemFact& Pattern, CFlags FieldMask) const
{
	if (!CMemFact::Match(Pattern, FieldMask)) FAIL;

	const CMemFactObstacle& PatternCast = (const CMemFactObstacle&)Pattern;

	if (pSourceStimulus.IsValid() && pSourceStimulus != PatternCast.pSourceStimulus) FAIL;

	OK;
}
//---------------------------------------------------------------------

} //namespace AI
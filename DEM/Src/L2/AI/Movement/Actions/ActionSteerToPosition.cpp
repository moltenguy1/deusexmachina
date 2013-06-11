#include "ActionSteerToPosition.h"

#include <AI/PropActorBrain.h>

namespace AI
{
__ImplementClass(AI::CActionSteerToPosition, 'ASTP', AI::CActionTraversePathEdge);

void CActionSteerToPosition::UpdatePathEdge(CActor* pActor, const CPathEdge* pEdge, const CPathEdge* pNextEdge)
{
	n_assert(pEdge);

	pActor->GetMotorSystem().SetDest(pEdge->Dest);
	pActor->GetMotorSystem().SetNextDest(pNextEdge ? pNextEdge->Dest : pEdge->Dest);
	pActor->SteeringType = (pEdge->IsLast || (pNextEdge && pNextEdge->IsLast)) ? AISteer_Type_Arrive : AISteer_Type_Seek;
}
//---------------------------------------------------------------------

//bool CActionSteerToPosition::Activate(CActor* pActor)
//{
//	OK;
//}
////---------------------------------------------------------------------

EExecStatus CActionSteerToPosition::Update(CActor* pActor)
{
	switch (pActor->MvmtStatus)
	{
		case AIMvmt_None:
		{
			// Restart movement here!
			//!!!None can be set if we can't move with the selected movement type!
			//???or AIMvmt_Failed for this case?
			return Running;
		}
		case AIMvmt_DestSet:	return Running;
		case AIMvmt_Done:		return Success;
		case AIMvmt_Stuck:
		{
			// Process stuck, now just
			return Failure;

			// Remember that stuck can be released, so we may want to wait for some time here
			// getting stuck time from BB / MotorSystem
		}
		default: n_error("CActionSteerToPosition::Update(): Unexpected movement status '%d'", pActor->MvmtStatus);
	}
	return Failure;
}
//---------------------------------------------------------------------

void CActionSteerToPosition::Deactivate(CActor* pActor)
{
	if (pActor->MvmtStatus == AIMvmt_DestSet || pActor->MvmtStatus == AIMvmt_Stuck)
		pActor->GetMotorSystem().ResetMovement();

	if (pActor->FacingStatus == AIFacing_DirSet)
		pActor->GetMotorSystem().ResetRotation();
}
//---------------------------------------------------------------------

} //namespace AI
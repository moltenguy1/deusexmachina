#include "ActionUseSmartObj.h"

#include <AI/PropActorBrain.h>
#include <AI/PropSmartObject.h>
#include <AI/Movement/Actions/ActionFace.h>
#include <Game/EntityManager.h>
#include <Game/GameServer.h>
#include <Events/EventServer.h>

namespace AI
{
__ImplementClass(AI::CActionUseSmartObj, 'AUSO', AI::CAction)

using namespace Prop;
using namespace Data;

bool CActionUseSmartObj::StartSOAction(CActor* pActor, Prop::CPropSmartObject* pSO)
{
	if (!pSO->IsActionAvailable(ActionID, pActor)) FAIL;
	Prop::CPropSmartObject::CAction* pSOAction = pSO->GetAction(ActionID);

	if (pSOAction->FreeUserSlots > 0) --pSOAction->FreeUserSlots;

	const CSmartAction& ActTpl = *pSOAction->pTpl;
	if (ActTpl.ProgressDriver == CSmartAction::PDrv_Duration)
	{
		CString AttrID("ActionProgress_");
		AttrID += ActionID.CStr();

		//!!!ActTpl.GetDuration()!
		Duration = ActTpl.Duration;
		Progress = ActTpl.ResetOnAbort() ? 0.f : pActor->GetEntity()->GetAttr<float>(CStrID(AttrID.CStr()), 0.f);

		if (ActTpl.TargetState.IsValid())
		{
			pSO->SetState(ActTpl.TargetState, ActionID, Duration, ActTpl.ManualTransitionControl());
			pSO->SetTransitionProgress(Progress);
		}
	}
	else if (ActTpl.ProgressDriver == CSmartAction::PDrv_SO_FSM)
	{
		n_assert_dbg(ActTpl.TargetState.IsValid());
		pSO->SetState(ActTpl.TargetState, ActionID);
		Duration = pSO->GetTransitionDuration();
		Progress = pSO->GetTransitionProgress();
	}
	else if (ActTpl.ProgressDriver == CSmartAction::PDrv_None)
	{
		//???if _TARGET_STATE
		//???	SO.FSM.SetState(_TARGET_STATE, _ACTION_ID, Auto)
		Duration = -1.f;
	}
	else n_error("CActionUseSmartObj::StartSOAction(): Unknown ProgressDriver!");

	Prop::CPropSmartObject* pActorSO = pActor->GetEntity()->GetProperty<Prop::CPropSmartObject>();
	if (pActorSO)
		pActorSO->SetState(CStrID("UsingSO"), ActionID); //!!!SYNC_ACTOR_ANIMATION ? Duration : -1.f

	PParams P = n_new(CParams(2)); //3));
	//P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
	P->Set(CStrID("SO"), TargetID);
	P->Set(CStrID("Action"), ActionID);
	pActor->GetEntity()->FireEvent(CStrID("OnSOActionStart"), P);

	if (Duration == 0.f) SetDone(pActor, ActTpl);

	OK;
}
//---------------------------------------------------------------------

DWORD CActionUseSmartObj::SetDone(CActor* pActor, const CSmartAction& ActTpl)
{
	WasDone = true;

	PParams P = n_new(CParams(2));
	//P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
	P->Set(CStrID("SO"), TargetID);
	P->Set(CStrID("Action"), ActionID);
	pActor->GetEntity()->FireEvent(CStrID("OnSOActionDone"), P);

	return ActTpl.EndOnDone() ? Success : Running;
}
//---------------------------------------------------------------------

bool CActionUseSmartObj::Activate(CActor* pActor)
{
	Game::CEntity* pSOEntity = EntityMgr->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) FAIL;
	CPropSmartObject* pSO = pSOEntity->GetProperty<CPropSmartObject>();
	if (!pSO) FAIL;

	WasDone = false;

	//!!!mb face externally, as go!
	Prop::CPropSmartObject::CAction* pSOAction = pSO->GetAction(ActionID);
	if (!pSOAction) FAIL;
	if (pSOAction->pTpl->FaceObject())
	{
		vector3 FaceDir = pSOEntity->GetAttr<matrix44>(CStrID("Transform")).Translation() - pActor->Position;
		FaceDir.norm();
		pActor->GetMotorSystem().SetFaceDirection(FaceDir);
		SubActFace = n_new(CActionFace);
		return SubActFace->Activate(pActor);
	}
	return StartSOAction(pActor, pSO);
}
//---------------------------------------------------------------------

DWORD CActionUseSmartObj::Update(CActor* pActor)
{
	//!!!IsValid() checks that values, second test may be not necessary!
	Game::CEntity* pSOEntity = EntityMgr->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) return Failure;
	CPropSmartObject* pSO = pSOEntity->GetProperty<CPropSmartObject>();
	if (!pSO) return Failure;

//!!!facing to GotoSO! can go-face-go-face-... until requested position and orientation are reached!
	// Finish SO facing if started
	if (SubActFace.IsValid())
	{
		DWORD Status = SubActFace->Update(pActor);
		switch (Status)
		{
			case Running: return Running;
			case Success:
				SubActFace->Deactivate(pActor);
				SubActFace = NULL;
				if (!StartSOAction(pActor, pSO)) return Failure;
				break;
			case Failure:
			default: //case Error:
				SubActFace->Deactivate(pActor);
				SubActFace = NULL;
				return Status;
		}
	}

	Prop::CPropSmartObject::CAction* pSOAction = pSO->GetAction(ActionID);
	if (!pSOAction) return Failure;
	const CSmartAction& ActTpl = *pSOAction->pTpl;

	DWORD Result = Running; //!!!if UpdateFunc() call it!
	if (Result == Failure || ExecResultIsError(Result)) return Result;
	if (WasDone) return Running;
	if (Result == Success) return SetDone(pActor, ActTpl);
	else if (ActTpl.ProgressDriver != CSmartAction::PDrv_None)
	{
		bool IsDone = false;
		float PrevProgress = Progress;
		if (ActTpl.ProgressDriver == CSmartAction::PDrv_Duration)
		{
			Progress += (float)GameSrv->GetFrameTime();
			if (ActTpl.TargetState.IsValid() && ActTpl.ManualTransitionControl())
				pSO->SetTransitionProgress(Progress);
			IsDone = (Progress >= Duration);
		}
		else if (ActTpl.ProgressDriver == CSmartAction::PDrv_SO_FSM)
		{
			if (pSO->IsInTransition())
			{
				if (pSO->GetTargetState() != ActTpl.TargetState || pSO->GetTransitionActionID() != ActionID) return Failure;
				Progress = pSO->GetTransitionProgress();
			}
			else
			{
				if (pSO->GetCurrState() != ActTpl.TargetState) return Failure;
				IsDone = true;
			}
		}

		if (IsDone) return SetDone(pActor, ActTpl);
		else if (ActTpl.SendProgressEvent() && Progress != PrevProgress)
		{
			PParams P = n_new(CParams(4));
			//P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
			P->Set(CStrID("SO"), TargetID);
			P->Set(CStrID("Action"), ActionID);
			P->Set(CStrID("PrevValue"), PrevProgress);
			P->Set(CStrID("Value"), Progress);
			pActor->GetEntity()->FireEvent(CStrID("OnSOActionProgress"), P);
		}
	}

	return Running;
}
//---------------------------------------------------------------------

void CActionUseSmartObj::Deactivate(CActor* pActor)
{
	if (SubActFace.IsValid())
	{
		SubActFace->Deactivate(pActor);
		SubActFace = NULL;
		return;
	}

	Game::CEntity* pSOEntity = EntityMgr->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) return;
	CPropSmartObject* pSO = pSOEntity->GetProperty<CPropSmartObject>();
	if (!pSO) return;
	Prop::CPropSmartObject::CAction* pSOAction = pSO->GetAction(ActionID);
	if (!pSOAction) return;
	const CSmartAction& ActTpl = *pSOAction->pTpl;

	PParams P = n_new(CParams(2));
	//P->Set(CStrID("Actor"), pActor->GetEntity()->GetUID());
	P->Set(CStrID("SO"), TargetID);
	P->Set(CStrID("Action"), ActionID);

	if (WasDone)
	{
		EventSrv->FireEvent(CStrID("OnSOActionEnd"), P);
		if (ActTpl.ProgressDriver == CSmartAction::PDrv_Duration && !ActTpl.ResetOnAbort())
		{
			CString AttrID("ActionProgress_");
			AttrID += ActionID.CStr();
			pActor->GetEntity()->DeleteAttr(CStrID(AttrID.CStr()));
		}
	}
	else
	{
		EventSrv->FireEvent(CStrID("OnSOActionAbort"), P);
		if (ActTpl.ProgressDriver == CSmartAction::PDrv_Duration && !ActTpl.ResetOnAbort())
		{
			CString AttrID("ActionProgress_");
			AttrID += ActionID.CStr();
			pActor->GetEntity()->SetAttr(CStrID(AttrID.CStr()), Progress);
		}
		if (ActTpl.TargetState.IsValid())
		{
			//if _RESET_ON_ABORT [? and drv is so fsm or (_SO_TIMING_MODE = Manual and _FREE_SLOTS = _MAX_SLOTS - 1)?]
			if (ActTpl.ResetOnAbort())
				pSO->AbortTransition(); //[?time/speed, not to switch anim immediately but to perform reverse transition?]
			else if (ActTpl.ProgressDriver == CSmartAction::PDrv_SO_FSM)
				pSO->StopTransition();
		}
	}

	// Abort might be caused by an actor state change, so don't affect that change
	Prop::CPropSmartObject* pActorSO = pActor->GetEntity()->GetProperty<Prop::CPropSmartObject>();
	if (pActorSO && pActorSO->GetCurrState() == CStrID("UsingSO"))
		pActorSO->SetState(CStrID("Idle"), ActionID);
	//???if in transition to UsingSO, reset to Idle too?

	if (pSOAction->FreeUserSlots >= 0) ++pSOAction->FreeUserSlots;
}
//---------------------------------------------------------------------

bool CActionUseSmartObj::IsValid(CActor* pActor) const
{
	Game::CEntity* pSOEntity = EntityMgr->GetEntity(TargetID);
	if (!pSOEntity || pSOEntity->GetLevel() != pActor->GetEntity()->GetLevel()) FAIL;
	CPropSmartObject* pSO = pSOEntity->GetProperty<CPropSmartObject>();
	if (!pSO) FAIL;
	return SubActFace.IsValid() ? SubActFace->IsValid(pActor) : pSO->IsActionEnabled(ActionID);
}
//---------------------------------------------------------------------

}
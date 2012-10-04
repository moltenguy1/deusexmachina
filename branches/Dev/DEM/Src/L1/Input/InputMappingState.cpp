#include "InputMappingState.h"

#include <Input/InputServer.h>
#include <Events/EventManager.h>
#include <Data/Params.h>

namespace Input
{

CInputMappingState::CInputMappingState(): CheckInOrder(false), SendStateChangeEvent(true), State(false)
{
}
//---------------------------------------------------------------------

bool CInputMappingState::Init(CStrID Name, const CParams& Desc)
{
	//!!!WRITE IT!
	OK;
}
//---------------------------------------------------------------------

void CInputMappingState::Enable()
{
	if (!IS_SUBSCRIBED(OnInputUpdated))
		DISP_SUBSCRIBE_PEVENT(InputSrv, OnInputUpdated, CInputMappingState, OnInputUpdated);
}
//---------------------------------------------------------------------

void CInputMappingState::Disable()
{
	UNSUBSCRIBE_EVENT(OnInputUpdated);
}
//---------------------------------------------------------------------

bool CInputMappingState::OnInputUpdated(const Events::CEventBase& Event)
{
	bool OldState = State;
	State = true;
	for (nArray<CCondition>::iterator It = Conditions.Begin(); It != Conditions.End(); It++)
	{
		bool CndState;
		switch (It->Type)
		{
			case CT_Key:		CndState = InputSrv->CheckKeyState(It->Key, It->KeyBtnState); break;
			case CT_MouseBtn:	CndState = InputSrv->CheckMouseBtnState(It->MouseBtn, It->KeyBtnState); break;
			case CT_Wheel:		CndState = (It->WheelFwd) ? InputSrv->GetWheelForward() > 0 : InputSrv->GetWheelBackward() > 0; break;
			default:			CndState = false;
		}

		It->State = CndState;

		State &= CndState;

		if (!CndState && CheckInOrder)
		{
			for (nArray<CCondition>::iterator It2 = Conditions.Begin(); It2 != It; It2++)
				It2->State = false;
			break;
		}
	}

	if (OldState != State && SendStateChangeEvent)
		EventMgr->FireEvent(CStrID((State) ? EventOn : EventOff));

	FAIL;
}
//---------------------------------------------------------------------

} // namespace Input

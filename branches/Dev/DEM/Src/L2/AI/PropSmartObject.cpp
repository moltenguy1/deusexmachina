#include "PropSmartObject.h"

#include <Game/Entity.h>
#include <Scripting/PropScriptable.h>
#include <AI/AIServer.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

namespace Prop
{
__ImplementClass(Prop::CPropSmartObject, 'PRSO', Game::CProperty);
__ImplementPropertyStorage(CPropSmartObject);

using namespace Data;

bool CPropSmartObject::InternalActivate()
{
	PParams Desc;
	
	const CString& DescResource = GetEntity()->GetAttr<CString>(CStrID("SmartObjDesc"), NULL);
	if (DescResource.IsValid()) Desc = DataSrv->LoadPRM(CString("Smarts:") + DescResource + ".prm");

	if (Desc.IsValid())
	{
		TypeID = Desc->Get(CStrID("TypeID"), CStrID::Empty);
		Movable = Desc->Get(CStrID("Movable"), false);

		PParams DescSection;
		if (Desc->Get<PParams>(DescSection, CStrID("Actions")))
		{
			Actions.BeginAdd(DescSection->GetCount());
			for (int i = 0; i < DescSection->GetCount(); i++)
			{
				const CParam& Prm = DescSection->Get(i);
				PParams ActDesc = Prm.GetValue<PParams>();
				LPCSTR TplName = ActDesc->Get<CString>(CStrID("Tpl")).CStr();
				const AI::CSmartObjActionTpl* pTpl = AISrv->GetSmartObjActionTpl(CStrID(TplName));
				if (pTpl) Actions.Add(Prm.GetName(), n_new(AI::CSmartObjAction)(*pTpl, ActDesc));
				else n_printf("AI, SO, Warning: can't find smart object action template '%s'\n", TplName);

				//!!!load action's Enabled and Progress!
			}
			Actions.EndAdd();
		}
	}

	Data::CData StateData;
	if (GetEntity()->GetAttr(StateData, CStrID("SOState")) || (Desc.IsValid() && Desc->Get(StateData, CStrID("DefaultState"))))
		CurrState = StateData.GetValue<CStrID>();
	else CurrState = CStrID::Empty;

	GetEntity()->SetAttr(CStrID("SOState"), CurrState);

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive())
	{
		EnableSI(*pProp);
		GetEntity()->FireEvent(CStrID("OnSOLoaded")); //???or in OnPropsActivated?
	}

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropSmartObject, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropSmartObject, OnPropDeactivating);
	OK;
}
//---------------------------------------------------------------------

void CPropSmartObject::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) DisableSI(*pProp);

	CurrState = CStrID::Empty;
	Actions.Clear();
}
//---------------------------------------------------------------------

bool CPropSmartObject::OnPropActivated(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		EnableSI(*(CPropScriptable*)pProp);
		GetEntity()->FireEvent(CStrID("OnSOLoaded")); //???or in OnPropsActivated?
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropSmartObject::OnPropDeactivating(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		DisableSI(*(CPropScriptable*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropSmartObject::SetState(CStrID ID)
{
	n_assert2(ID.IsValid(), "CPropSmartObject::SetState() > Tried to set empty state");

	//!!!need to know what states are defined for this object!

	if (ID == CurrState) OK;

	PParams P = n_new(CParams(2));
	P->Set(CStrID("From"), CurrState);
	P->Set(CStrID("To"), ID);

	if (CurrState.IsValid()) GetEntity()->FireEvent(CStrID("OnSOStateLeave"), P);
	CurrState = ID;
	GetEntity()->SetAttr(CStrID("SOState"), CurrState);
	GetEntity()->FireEvent(CStrID("OnSOStateEnter"), P);

	OK;
}
//---------------------------------------------------------------------

void CPropSmartObject::EnableAction(CStrID ID, bool Enable)
{
	int Idx = Actions.FindIndex(ID);
	if (Idx == INVALID_INDEX) return;

	AI::CSmartObjAction* pAct = Actions.ValueAt(Idx).GetUnsafe();
	if (pAct->Enabled == Enable) return;
	pAct->Enabled = Enable;

	PParams P = n_new(CParams(2));
	P->Set(CStrID("ActionID"), ID);
	P->Set(CStrID("Enabled"), Enable);
	GetEntity()->FireEvent(CStrID("OnSOActionAvailabile"), P);
}
//---------------------------------------------------------------------

bool CPropSmartObject::GetDestinationParams(CStrID ActionID, float ActorRadius, vector3& OutOffset, float& OutMinDist, float& OutMaxDist)
{
	AI::PSmartObjAction Action = GetAction(ActionID);

	if (Action.IsValid())
	{
		matrix33 Tfm = GetEntity()->GetAttr<matrix44>(CStrID("Transform")).ToMatrix33();
		Tfm.mult(Action->GetTpl().DestOffset, OutOffset);
		OutMinDist = Action->GetTpl().MinDistance;
		OutMaxDist = Action->GetTpl().MaxDistance;
		if (Action->ActorRadiusMatters())
		{
			OutMinDist += ActorRadius;
			OutMaxDist += ActorRadius;
		}
		//???add SORadiusMatters? for items, enemies etc
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

}
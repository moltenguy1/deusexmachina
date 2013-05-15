#include "PropTrigger.h"

#include <Game/EntityManager.h>
#include <Game/GameServer.h>
#include <Scripting/Prop/PropScriptable.h>
#include <Physics/PhysicsServer.h>
#include <Physics/PhysicsLevel.h>
#include <Events/Subscription.h>
#include <Render/DebugDraw.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>

namespace Attr
{
	DefineInt(TrgShapeType);
	DefineFloat4(TrgShapeParams);
	DefineFloat(TrgPeriod);
	DefineBool(TrgEnabled);
	DefineFloat(TrgTimeLastTriggered);
};

BEGIN_ATTRS_REGISTRATION(PropTrigger)
	RegisterIntWithDefault(TrgShapeType, ReadOnly, (int)Physics::CShape::Sphere);
	RegisterFloat4WithDefault(TrgShapeParams, ReadOnly, vector4(1.f, 1.f, 1.f, 1.f));
	RegisterFloat(TrgPeriod, ReadOnly);
	RegisterBool(TrgEnabled, ReadWrite);
	RegisterFloat(TrgTimeLastTriggered, ReadWrite);
END_ATTRS_REGISTRATION

namespace Properties
{
__ImplementClassNoFactory(Properties::CPropTrigger, CPropTransformable);
__ImplementClass(Properties::CPropTrigger);
RegisterProperty(CPropTrigger);

using namespace Physics;

static nString OnTriggerEnter("OnTriggerEnter");
static nString OnTriggerApply("OnTriggerApply");
static nString OnTriggerLeave("OnTriggerLeave");

CPropTrigger::~CPropTrigger()
{
}
//---------------------------------------------------------------------

void CPropTrigger::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	Attrs.Append(Attr::TrgShapeType);
	Attrs.Append(Attr::TrgShapeParams);
	Attrs.Append(Attr::TrgPeriod);
	Attrs.Append(Attr::TrgEnabled);
	Attrs.Append(Attr::TrgTimeLastTriggered);
	CPropTransformable::GetAttributes(Attrs);
}
//---------------------------------------------------------------------

void CPropTrigger::Activate()
{
	CPropTransformable::Activate();

	Period = GetEntity()->Get<float>(Attr::TrgPeriod);
	if (Period > 0.f) TimeLastTriggered = GetEntity()->Get<float>(Attr::TrgTimeLastTriggered);

	const vector4& ShapeParams = GetEntity()->Get<vector4>(Attr::TrgShapeParams);
	switch (GetEntity()->Get<int>(Attr::TrgShapeType))
	{
		case CShape::Box:
			pCollShape = (CShape*)PhysicsSrv->CreateBoxShape(GetEntity()->Get<matrix44>(Attr::Transform),
				InvalidMaterial, vector3(ShapeParams.x, ShapeParams.y, ShapeParams.z));
			break;
		case CShape::Sphere:
			pCollShape = (CShape*)PhysicsSrv->CreateSphereShape(GetEntity()->Get<matrix44>(Attr::Transform),
				InvalidMaterial, ShapeParams.x);
			break;
		case CShape::Capsule:
			pCollShape = (CShape*)PhysicsSrv->CreateCapsuleShape(GetEntity()->Get<matrix44>(Attr::Transform),
				InvalidMaterial, ShapeParams.x, ShapeParams.y);
			break;
		default: n_error("Entity '%s': CPropTrigger::Activate(): Shape type %d unsupported\n",
					 GetEntity()->GetUID().CStr(),
					 GetEntity()->Get<int>(Attr::TrgShapeType));
	}

	n_assert(pCollShape);
	pCollShape->SetCategoryBits(Physics::None);
	pCollShape->SetCollideBits(Physics::None);

	pCollShape->Attach(PhysicsSrv->GetLevel()->GetODEDynamicSpaceID()); //???here or when enabled only?
	SetEnabled(GetEntity()->Get<bool>(Attr::TrgEnabled));

	PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropTrigger, OnPropsActivated);
	PROP_SUBSCRIBE_PEVENT(ExposeSI, CPropTrigger, ExposeSI);
	PROP_SUBSCRIBE_PEVENT(OnSave, CPropTrigger, OnSave);
	PROP_SUBSCRIBE_PEVENT(OnRenderDebug, CPropTrigger, OnRenderDebug);
}
//---------------------------------------------------------------------

void CPropTrigger::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(ExposeSI);
	UNSUBSCRIBE_EVENT(OnSave);
	UNSUBSCRIBE_EVENT(OnRenderDebug);

	SetEnabled(false);
	pCollShape->Detach();
	pCollShape = NULL;

	CPropTransformable::Deactivate();
}
//---------------------------------------------------------------------

void CPropTrigger::SetEnabled(bool Enable)
{
	if (Enabled == Enable) return;
	if (Enable)
	{
		//???reset timestamp?
		//pCollShape->Attach(PhysicsSrv->GetLevel()->GetODEDynamicSpaceID());
		PROP_SUBSCRIBE_PEVENT(OnBeginFrame, CPropTrigger, OnBeginFrame);
	}
	else
	{
		UNSUBSCRIBE_EVENT(OnBeginFrame);
		//pCollShape->Detach();
	}
	Enabled = Enable;
}
//---------------------------------------------------------------------

bool CPropTrigger::OnPropsActivated(const CEventBase& Event)
{
	CPropScriptable* pScriptable = GetEntity()->GetProperty<CPropScriptable>();
	pScriptObj = pScriptable ? pScriptable->GetScriptObject() : NULL;
	OK;
}
//---------------------------------------------------------------------

bool CPropTrigger::OnBeginFrame(const Events::CEventBase& Event)
{
	nArray<Game::PEntity> *pInsideNow, *pInsideLastFrame;
	if (SwapArrays)
	{
		pInsideNow = &EntitiesInsideLastFrame;
		pInsideLastFrame = &EntitiesInsideNow;
	}
	else
	{
		pInsideNow = &EntitiesInsideNow;
		pInsideLastFrame = &EntitiesInsideLastFrame;
	}

	pCollShape->SetCategoryBits(Physics::Trigger);
	pCollShape->SetCollideBits(Physics::Dynamic); //!!!later set Trigger bit only on entities who want to collide with trigger!

	nArray<CContactPoint> Contacts;
	pCollShape->Collide(Physics::CFilterSet(), Contacts);
	
	pCollShape->SetCategoryBits(Physics::None);
	pCollShape->SetCollideBits(Physics::None);

	uint Stamp = PhysicsSrv->GetUniqueStamp();
	pInsideNow->Clear();
	for (int i = 0; i < Contacts.GetCount(); i++)
	{
		Physics::CEntity* pPhysEnt = Contacts[i].GetEntity();
		if (pPhysEnt && pPhysEnt->GetStamp() != Stamp)
		{
			pPhysEnt->SetStamp(Stamp);
			Game::CEntity* pEnt = EntityMgr->GetEntity(pPhysEnt->GetUserData());
			if (pEnt)
			{
				pInsideNow->Append(pEnt); //???!!!sort to faster search?!
				if (!pInsideLastFrame->Contains(pEnt))
				{
					if (pScriptObj)
					{
						//???notify pEnt too by local or even global event?
						pScriptObj->RunFunctionData(OnTriggerEnter.CStr(), nString(pEnt->GetUID().CStr()));
						//???here or from OnTriggerEnter if needed for this trigger?
						//pScriptObj->RunFunctionData(OnTriggerApply, nString(pEnt->GetUID().CStr()))
					}
				}
			}
		}
	}

	for (int i = 0; i < pInsideLastFrame->GetCount(); i++)
	{
		Game::CEntity* pEnt = pInsideLastFrame->At(i);
		if (!pInsideNow->Contains(pEnt) && pEnt->IsActive())
			if (pScriptObj)
				pScriptObj->RunFunctionData(OnTriggerLeave.CStr(), nString(pEnt->GetUID().CStr()));
	}

	if (Period > 0.f && GameSrv->GetTime() - TimeLastTriggered >= Period)
	{
		for (int i = 0; i < pInsideNow->GetCount(); i++)
			if (pScriptObj)
				pScriptObj->RunFunctionData(OnTriggerLeave.CStr(), nString(pInsideNow->At(i)->GetUID().CStr()));
		TimeLastTriggered = (float)GameSrv->GetTime();
	}

	SwapArrays = !SwapArrays;

	OK;
}
//---------------------------------------------------------------------

bool CPropTrigger::OnSave(const CEventBase& Event)
{
	GetEntity()->Set<bool>(Attr::TrgEnabled, Enabled);
	if (Period > 0.f) GetEntity()->Set<float>(Attr::TrgTimeLastTriggered, TimeLastTriggered);
	OK;
}
//---------------------------------------------------------------------

bool CPropTrigger::OnRenderDebug(const Events::CEventBase& Event)
{
	static const vector4 ColorOn(0.9f, 0.58f, 1.0f, 0.3f); // purple
	static const vector4 ColorOff(0.0f, 0.0f, 0.0f, 0.08f); // black
	
	const vector4& ShapeParams = GetEntity()->Get<vector4>(Attr::TrgShapeParams);
	matrix44 Tfm;
	switch (GetEntity()->Get<int>(Attr::TrgShapeType))
	{
		case CShape::Box:
			Tfm.scale(vector3(ShapeParams.x, ShapeParams.y, ShapeParams.z));
			Tfm *= GetEntity()->Get<matrix44>(Attr::Transform);
			DebugDraw->DrawBox(Tfm, Enabled ? ColorOn : ColorOff);
			break;
		case CShape::Sphere:
			DebugDraw->DrawSphere(GetEntity()->Get<matrix44>(Attr::Transform).Translation(), ShapeParams.x, Enabled ? ColorOn : ColorOff);
			break;
		default: break; //!!!capsule rendering!
	}

	OK;
}
//---------------------------------------------------------------------

} // namespace Properties

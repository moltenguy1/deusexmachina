#include "PropActorPhysics.h"

#include <Game/Entity.h>
#include <Physics/Level.h>
#include <Physics/CharEntity.h>
#include <Render/DebugDraw.h>
#include <Loading/EntityFactory.h>

namespace Attr
{
	DeclareAttr(Physics);
	DeclareAttr(VelocityVector);
	DeclareAttr(Radius);
	DeclareAttr(Height);
}

namespace Properties
{
ImplementRTTI(Properties::CPropActorPhysics, Properties::CPropAbstractPhysics);
ImplementFactory(Properties::CPropActorPhysics);
RegisterProperty(CPropActorPhysics);

void CPropActorPhysics::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	CPropAbstractPhysics::GetAttributes(Attrs);
	Attrs.Append(Attr::Physics);
	Attrs.Append(Attr::VelocityVector);
}
//------------------------------------------------------------------------------

void CPropActorPhysics::Activate()
{
	CPropAbstractPhysics::Activate();

	PROP_SUBSCRIBE_PEVENT(OnMoveAfter, CPropActorPhysics, OnMoveAfter);
	PROP_SUBSCRIBE_PEVENT(OnEntityRenamed, CPropActorPhysics, OnEntityRenamed);
	PROP_SUBSCRIBE_PEVENT(AIBodyRequestLVelocity, CPropActorPhysics, OnRequestLinearVelocity);
	PROP_SUBSCRIBE_PEVENT(AIBodyRequestAVelocity, CPropActorPhysics, OnRequestAngularVelocity);
}
//---------------------------------------------------------------------

void CPropActorPhysics::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnMoveAfter);
	UNSUBSCRIBE_EVENT(OnEntityRenamed);
	UNSUBSCRIBE_EVENT(AIBodyRequestLVelocity);
	UNSUBSCRIBE_EVENT(AIBodyRequestAVelocity);

	CPropAbstractPhysics::Deactivate();
}
//---------------------------------------------------------------------

Physics::CEntity* CPropActorPhysics::GetPhysicsEntity() const
{
	return PhysEntity;
}
//---------------------------------------------------------------------

void CPropActorPhysics::EnablePhysics()
{
	n_assert(!IsEnabled());

	PhysEntity = Physics::CCharEntity::Create();
	PhysEntity->SetUserData(GetEntity()->GetUniqueID());
	PhysEntity->SetTransform(GetEntity()->Get<matrix44>(Attr::Transform));
	PhysEntity->CompositeName = GetEntity()->Get<nString>(Attr::Physics);
	PhysEntity->Radius = GetEntity()->Get<float>(Attr::Radius);
	PhysEntity->Height = GetEntity()->Get<float>(Attr::Height);
	PhysEntity->Hover = 0.2f;

	//!!!recreate physics capsule on R/H change!

	PhysicsSrv->GetLevel()->AttachEntity(PhysEntity);

	Stop();

	CPropAbstractPhysics::EnablePhysics();
}
//------------------------------------------------------------------------------

void CPropActorPhysics::DisablePhysics()
{
	n_assert(IsEnabled());

	Stop();
	PhysicsSrv->GetLevel()->RemoveEntity(PhysEntity);
	PhysEntity = NULL;

	CPropAbstractPhysics::DisablePhysics();
}
//------------------------------------------------------------------------------

void CPropActorPhysics::Stop()
{
	PhysEntity->SetDesiredLinearVelocity(vector3::Zero);
	GetEntity()->Set<vector4>(Attr::VelocityVector, vector4::Zero);
}
//------------------------------------------------------------------------------

void CPropActorPhysics::SetTransform(const matrix44& NewTF)
{
	PhysEntity->SetTransform(NewTF);
	if (!IsEnabled()) CPropAbstractPhysics::SetTransform(NewTF);
}
//---------------------------------------------------------------------

// The OnMoveAfter() method transfers the current physics entity transform to the game entity.
bool CPropActorPhysics::OnMoveAfter(const Events::CEventBase& Event)
{
	if (IsEnabled() && PhysEntity->HasTransformChanged())
	{
		CPropAbstractPhysics::SetTransform(PhysEntity->GetTransform());
		GetEntity()->Set<vector3>(Attr::VelocityVector, PhysEntity->GetVelocity());
	}

	OK;
}
//------------------------------------------------------------------------------

bool CPropActorPhysics::OnEntityRenamed(const Events::CEventBase& Event)
{
	if (PhysEntity.isvalid()) PhysEntity->SetUserData(GetEntity()->GetUniqueID());
	OK;
}
//---------------------------------------------------------------------

bool CPropActorPhysics::OnRequestLinearVelocity(const Events::CEventBase& Event)
{
	const vector4& Velocity = (*((Events::CEvent&)Event).Params).Get<vector4>(CStrID("Velocity"));
	PhysEntity->SetDesiredLinearVelocity(vector3(Velocity.x, Velocity.y, Velocity.z));
	OK;
}
//---------------------------------------------------------------------

bool CPropActorPhysics::OnRequestAngularVelocity(const Events::CEventBase& Event)
{
	PhysEntity->SetDesiredAngularVelocity((*((Events::CEvent&)Event).Params).Get<float>(CStrID("Velocity")));
	OK;
}
//---------------------------------------------------------------------

void CPropActorPhysics::OnRenderDebug()
{
	static const vector4 ColorVel(1.0f, 0.5f, 0.0f, 1.0f);
	static const vector4 ColorDesVel(0.0f, 1.0f, 1.0f, 1.0f);

	const matrix44& Tfm = GetEntity()->Get<matrix44>(Attr::Transform);

	DebugDraw->DrawCoordAxes(Tfm);
	DebugDraw->DrawLine(Tfm.pos_component(), Tfm.pos_component() + PhysEntity->GetVelocity(), ColorVel);
	DebugDraw->DrawLine(Tfm.pos_component(), Tfm.pos_component() + PhysEntity->GetDesiredLinearVelocity(), ColorDesVel);

	if (GetEntity()->GetUniqueID() == CStrID("GG"))
	{
		nString text;
		text.Format("Velocity: %.4f, %.4f, %.4f\nDesired velocity: %.4f, %.4f, %.4f\nAngular desired: %.5f\n"
					"Speed: %.4f\nDesired speed: %.4f",
			PhysEntity->GetVelocity().x,
			PhysEntity->GetVelocity().y,
			PhysEntity->GetVelocity().z,
			PhysEntity->GetDesiredLinearVelocity().x,
			PhysEntity->GetDesiredLinearVelocity().y,
			PhysEntity->GetDesiredLinearVelocity().z,
			PhysEntity->GetDesiredAngularVelocity(),
			PhysEntity->GetVelocity().len(),
			PhysEntity->GetDesiredLinearVelocity().len());
		vector4 textColor(1.0f, 1.0f, 1.0f, 1.0f);
		//rectangle textRect(vector2(0.5f, 0.0f), vector2(1.0f, 1.0f));
		//uint textFlags = Top | Left | NoClip | ExpandTabs;
		//GFX
		//nGfxServer2::Instance()->DrawText(text, textColor, textRect, textFlags, false);
	}
}
//---------------------------------------------------------------------

} // namespace Properties
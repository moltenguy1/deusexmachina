#include "PhysicsServerOld.h"

#include <Physics/PhysicsWorldOld.h>
#include <Physics/Composite.h>
#include <Physics/Collision/BoxShape.h>
#include <Physics/Collision/SphereShape.h>
#include <Physics/Collision/CapsuleShape.h>
#include <Physics/Collision/MeshShape.h>
#include <Physics/Collision/HeightfieldShapeOld.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

namespace Physics
{
__ImplementClass(Physics::CPhysicsServerOld, 'PHSO', Core::CRefCounted);
__ImplementSingleton(Physics::CPhysicsServerOld);

uint CPhysicsServerOld::UniqueStamp = 0;

static const nString PhysClassPrefix("Physics::C");

CPhysicsServerOld::CPhysicsServerOld():
	isOpen(false),
	Contacts(256, 256),
	Entities(1024, 1024)
{
	__ConstructSingleton;
	Contacts.Flags.Set(Array_DoubleGrowSize);
}
//---------------------------------------------------------------------

CPhysicsServerOld::~CPhysicsServerOld()
{
	n_assert(!isOpen);
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CPhysicsServerOld::Open()
{
	n_assert(!isOpen);
	CMaterialTable::Setup();
	isOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CPhysicsServerOld::Close()
{
	n_assert(isOpen);
	isOpen = false;
}
//---------------------------------------------------------------------

CBoxShape* CPhysicsServerOld::CreateBoxShape(const matrix44& TF, CMaterialType MatType, const vector3& Size) const
{
	CBoxShape* Shape = CBoxShape::CreateInstance();
	Shape->SetTransform(TF);
	Shape->SetMaterialType(MatType);
	Shape->SetSize(Size);
	return Shape;
}
//---------------------------------------------------------------------

CSphereShape* CPhysicsServerOld::CreateSphereShape(const matrix44& TF, CMaterialType MatType, float Radius) const
{
	CSphereShape* Shape = CSphereShape::CreateInstance();
	Shape->SetTransform(TF);
	Shape->SetMaterialType(MatType);
	Shape->SetRadius(Radius);
	return Shape;
}
//---------------------------------------------------------------------

CCapsuleShape* CPhysicsServerOld::CreateCapsuleShape(const matrix44& TF, CMaterialType MatType,
												  float Radius, float Length) const
{
	CCapsuleShape* Shape = CCapsuleShape::CreateInstance();
	Shape->SetTransform(TF);
	Shape->SetMaterialType(MatType);
	Shape->SetRadius(Radius);
	Shape->SetLength(Length);
	return Shape;
}
//---------------------------------------------------------------------

CMeshShape* CPhysicsServerOld::CreateMeshShape(const matrix44& TF, CMaterialType MatType, const nString& FileName) const
{
	CMeshShape* Shape = CMeshShape::CreateInstance();
	Shape->SetTransform(TF);
	Shape->SetMaterialType(MatType);
	Shape->SetFileName(FileName);
	return Shape;
}
//---------------------------------------------------------------------

CHeightfieldShapeOld* CPhysicsServerOld::CreateHeightfieldShape(const matrix44& TF, CMaterialType MatType, const nString& FileName) const
{
	CHeightfieldShapeOld* Shape = CHeightfieldShapeOld::CreateInstance();
	Shape->SetTransform(TF);
	Shape->SetMaterialType(MatType);
	Shape->SetFileName(FileName);
	return Shape;
}
//---------------------------------------------------------------------

//!!!???to loaders?!
CComposite* CPhysicsServerOld::LoadCompositeFromPRM(const nString& Name) const
{
	n_assert(Name.IsValid());
	PParams Desc = DataSrv->LoadPRM(nString("physics:") + Name + ".prm");
	if (!Desc.IsValid()) return NULL;

	CComposite* pComposite = (CComposite*)Factory->Create(PhysClassPrefix + Desc->Get<nString>(CStrID("Type")));
	if (!pComposite) return NULL;

	int Idx = Desc->IndexOf(CStrID("Bodies"));
	if (Idx != INVALID_INDEX)
	{
		CDataArray& Bodies = *Desc->Get<PDataArray>(Idx);
		pComposite->BeginBodies(Bodies.GetCount());
		for (int i = 0; i < Bodies.GetCount(); i++)
		{
			PParams BodyDesc = Bodies[i];
			PRigidBody pBody = CRigidBody::CreateInstance();
			pBody->Name = BodyDesc->Get<nString>(CStrID("Name"));
			pBody->CollideConnected = BodyDesc->Get<bool>(CStrID("CollideConnected"), false);
			matrix44 InitialTfm(quaternion(
				BodyDesc->Get<float>(CStrID("RotX")),
				BodyDesc->Get<float>(CStrID("RotY")),
				BodyDesc->Get<float>(CStrID("RotZ")),
				BodyDesc->Get<float>(CStrID("RotW"))));
			InitialTfm.translate(vector3(
				BodyDesc->Get<float>(CStrID("PosX")),
				BodyDesc->Get<float>(CStrID("PosY")),
				BodyDesc->Get<float>(CStrID("PosZ"))));
			pBody->SetInitialTransform(InitialTfm);
			pBody->SetLinkName(CRigidBody::ModelNode, BodyDesc->Get<nString>(CStrID("Model"), NULL));
			pBody->SetLinkName(CRigidBody::ShadowNode, BodyDesc->Get<nString>(CStrID("Shadow"), NULL));
			pBody->SetLinkName(CRigidBody::JointNode, BodyDesc->Get<nString>(CStrID("Joint"), NULL));

			CDataArray& Shapes = *BodyDesc->Get<PDataArray>(CStrID("Shapes"));
			pBody->BeginShapes(Shapes.GetCount());
			for (int j = 0; j < Shapes.GetCount(); j++)
			{
				PParams ShapeDesc = Shapes[j];
				PShape pShape = (CShape*)Factory->Create(PhysClassPrefix + ShapeDesc->Get<nString>(CStrID("Type")));
				pShape->Init(ShapeDesc);
				pBody->AddShape(pShape);
			}
			pBody->EndShapes();

			pComposite->AddBody(pBody);
		}
		pComposite->EndBodies();
	}
	
	Idx = Desc->IndexOf(CStrID("Joints"));
	if (Idx != INVALID_INDEX)
	{
		CDataArray& Joints = *Desc->Get<PDataArray>(Idx);
		pComposite->BeginJoints(Joints.GetCount());
		for (int i = 0; i < Joints.GetCount(); i++)
		{
			PParams JointDesc = Joints[i];
			PJoint pJoint = (CJoint*)Factory->Create(PhysClassPrefix + JointDesc->Get<nString>(CStrID("Type")));
			Idx = Desc->IndexOf(CStrID("Body1"));
			if (Idx != INVALID_INDEX)
				pJoint->SetBody1(pComposite->GetBodyByName(Desc->Get(Idx).GetRawValue()));
			Idx = Desc->IndexOf(CStrID("Body2"));
			if (Idx != INVALID_INDEX)
				pJoint->SetBody2(pComposite->GetBodyByName(Desc->Get(Idx).GetRawValue()));
			pJoint->LinkName = Desc->Get<nString>(CStrID("Joint"), NULL);
			pJoint->Init(JointDesc);
			pComposite->AddJoint(pJoint);
		}
		pComposite->EndJoints();
	}

	Idx = Desc->IndexOf(CStrID("Shapes"));
	if (Idx != INVALID_INDEX)
	{
		CDataArray& Shapes = *Desc->Get<PDataArray>(Idx);
		pComposite->BeginShapes(Shapes.GetCount());
		for (int i = 0; i < Shapes.GetCount(); i++)
		{
			PParams ShapeDesc = Shapes[i];
			PShape pShape = (CShape*)Factory->Create("Physics::C" + ShapeDesc->Get<nString>(CStrID("Type")));
			pShape->Init(ShapeDesc);
			pComposite->AddShape(pShape);
		}
		pComposite->EndShapes();
	}

	return pComposite;
}
//---------------------------------------------------------------------

//!!!TO LEVEL!
int CPhysicsServerOld::GetEntitiesInShape(PShape Shape, const CFilterSet& ExcludeSet,
									   nArray<PEntity>& Result)
{
	/*
	n_assert(CurrLevel);

	Shape->Attach(CurrLevel->GetODEDynamicSpaceID());
	Contacts.Reset();
	Shape->Collide(ExcludeSet, Contacts);
	Shape->Detach();

	int OldResultSize = Result.GetCount();
	
	//???stamp?
	uint Stamp = GetUniqueStamp();
	for (int i = 0; i < Contacts.GetCount(); i++)
	{
		CEntity* pEnt = Contacts[i].GetEntity();
		if (pEnt && pEnt->GetStamp() != Stamp)
		{
			pEnt->SetStamp(Stamp);
			Result.Append(pEnt);
		}
	}
	return Result.GetCount() - OldResultSize;
	*/
	return 0;
}
//---------------------------------------------------------------------

// This method returns all physics entities touching the given spherical
// area. The method creates a sphere shape and calls its collide
// method, so it's quite fast. Note that entities will be appended to the
// array, so usually you should make sure to pass an empty array. This method
// will also overwrite the internal Contacts array which can be
// queried after the method has returned, but note that there will only
// be one contact per physics shape.
int CPhysicsServerOld::GetEntitiesInSphere(const vector3& Pos, float Radius, const CFilterSet& ExcludeSet,
										nArray<PEntity>& Result)
{
	n_assert(Radius >= 0.0f); //???in shape constructor?
	
	matrix44 Tfm;
	Tfm.translate(Pos);
	
	return GetEntitiesInShape(CreateSphereShape(Tfm, InvalidMaterial, Radius), ExcludeSet, Result);
}
//---------------------------------------------------------------------

int CPhysicsServerOld::GetEntitiesInBox(const vector3& Scale, const matrix44& TF, const CFilterSet& ExcludeSet,
									 nArray<PEntity>& Result)
{
	n_assert(Scale.x >= 0 && Scale.y >= 0 && Scale.z >= 0); //???in shape constructor?

	//???can optimize?
	matrix44 Tfm;
	Tfm.AxisX() = TF.AxisX() / Scale.x;
	Tfm.AxisY() = TF.AxisY() / Scale.y;
	Tfm.AxisZ() = TF.AxisZ() / Scale.z;
	Tfm.set_translation(TF.Translation());

	return GetEntitiesInShape(CreateBoxShape(Tfm, InvalidMaterial, Scale), ExcludeSet, Result);
}
//---------------------------------------------------------------------

} // namespace Physics

#include "EntityManager.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Events/EventManager.h>

namespace Game
{
__ImplementClassNoFactory(Game::CEntityManager, Core::CRefCounted);
__ImplementSingleton(Game::CEntityManager);

CEntityManager::~CEntityManager()
{
	DeleteAllEntities();

	// Delete storages allocated in RegisterProperty and clear storage refs in prop's static fields.
	// We don't need to deactivate and detach properties, DeleteAllEntities deleted all prop instances.
	for (int i = 0; i < PropStorages.GetCount(); ++i)
	{
		n_assert_dbg(!(*PropStorages.ValueAt(i))->GetCount());
		n_delete(*PropStorages.ValueAt(i));
		*PropStorages.ValueAt(i) = NULL;
	}
	PropStorages.Clear();

	__DestructSingleton;
}
//---------------------------------------------------------------------

PEntity CEntityManager::CreateEntity(CStrID UID, CGameLevel& Level)
{
	n_assert(UID.IsValid());
	n_assert(!EntityExists(UID)); //???return NULL or existing entity?
	PEntity Entity = n_new(CEntity(UID));
	Entity->SetLevel(&Level);
	Entities.Append(Entity);
	UIDToEntity.Add(Entity->GetUID(), Entity.GetUnsafe());
	return Entity;
}
//---------------------------------------------------------------------

bool CEntityManager::RenameEntity(CEntity& Entity, CStrID NewUID)
{
	if (!NewUID.IsValid()) FAIL;
	if (Entity.GetUID() == NewUID) OK;
	if (EntityExists(NewUID)) FAIL;
	UIDToEntity.Erase(Entity.GetUID());
	Entity.SetUID(NewUID);
	UIDToEntity.Add(Entity.GetUID(), &Entity);
	Entity.FireEvent(CStrID("OnEntityRenamed"));
	OK;
}
//---------------------------------------------------------------------

PEntity CEntityManager::CloneEntity(const CEntity& Entity, CStrID UID)
{
	// create entity with new UID
	// copy attributes
	// attach all the same properties
	// if props have copy constructor or Clone method, exploit it
	n_error("CEntityManager::CloneEntity() -> IMPLEMENT ME!!!");
	return NULL;
}
//---------------------------------------------------------------------

void CEntityManager::DeleteEntity(int Idx)
{
	CEntity& Entity = *Entities[Idx];

	n_assert_dbg(!Entity.IsActivating() && !Entity.IsDeactivating());

	if (Entity.IsActive()) Entity.Deactivate();

	// Remove all properties of this entity
	for (int i = 0; i < PropStorages.GetCount(); ++i)
		(*PropStorages.ValueAt(i))->Erase(Entity.GetUID());

	UIDToEntity.Erase(Entity.GetUID());
	Entities.EraseAt(Idx);
}
//---------------------------------------------------------------------

void CEntityManager::DeleteEntities(const CGameLevel& Level)
{
	for (int i = Entities.GetCount() - 1; i >= 0; --i)
		if (&Entities[i]->GetLevel() == &Level)
			DeleteEntity(i);
}
//---------------------------------------------------------------------

CEntity* CEntityManager::GetEntity(CStrID UID, bool SearchInAliases) const
{
	if (SearchInAliases)
	{
		int Idx = Aliases.FindIndex(UID);
		if (Idx != INVALID_INDEX) UID = Aliases.ValueAt(Idx);
	}

	CEntity* pEnt = NULL;
	UIDToEntity.Get(UID, pEnt);
	return pEnt;
}
//---------------------------------------------------------------------

void CEntityManager::GetEntitiesByLevel(CStrID LevelID, nArray<CEntity*>& Out) const
{
	for (int i = 0; i < Entities.GetCount(); ++i)
	{
		CEntity* pEntity = Entities[i].GetUnsafe();
		if (pEntity && pEntity->GetLevel().GetID() == LevelID)
			Out.Append(pEntity);
	}
}
//---------------------------------------------------------------------

CProperty* CEntityManager::AttachProperty(CEntity& Entity, const Core::CRTTI* pRTTI) const
{
	if (!pRTTI) return NULL;

	/* // Another way to get storage, without early instantiation. ???What is better?
	CPropertyStorage* pStorage = NULL;
	const Core::CRTTI* pCurrRTTI = pRTTI;
	while (pCurrRTTI)
	{
		int Idx = PropStorages.FindIndex(pRTTI);
		if (Idx != INVALID_INDEX)
		{
			pStorage = *PropStorages.ValueAt(Idx);
			break;
		}
		pCurrRTTI = pCurrRTTI->GetParent();
	}
	*/

	PProperty Prop = (CProperty*)pRTTI->CreateInstance();
	CPropertyStorage* pStorage = Prop->GetStorage();
	n_assert2_dbg(pStorage, (nString("Property ") + pRTTI->GetName() + " is not registered!").CStr());

	if (!pStorage->Get(Entity.GetUID(), Prop))
	{
		pStorage->Add(Entity.GetUID(), Prop);
		Prop->SetEntity(&Entity);
	}
	return Prop.GetUnsafe();
}
//---------------------------------------------------------------------

void CEntityManager::RemoveProperty(CEntity& Entity, Core::CRTTI& Type) const
{
	int Idx = PropStorages.FindIndex(&Type);
	n_assert2_dbg(Idx != INVALID_INDEX, (nString("Property ") + Type.GetName() + " is not registered!").CStr());
	if (Idx == INVALID_INDEX) return;
	CPropertyStorage* pStorage = *PropStorages.ValueAt(Idx);
	n_assert_dbg(pStorage);
	pStorage->Erase(Entity.GetUID());
}
//---------------------------------------------------------------------

void CEntityManager::GetPropertiesOfEntity(CStrID EntityID, nArray<CProperty*>& Out) const
{
	for (int i = 0; i < PropStorages.GetCount(); ++i)
	{
		PProperty Prop;
		if ((*PropStorages.ValueAt(i))->Get(EntityID, Prop))
			Out.Append(Prop.GetUnsafe());
	}
}
//---------------------------------------------------------------------

}
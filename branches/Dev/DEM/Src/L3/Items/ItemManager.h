#pragma once
#ifndef __IPG_ITEM_MANAGER_H__
#define __IPG_ITEM_MANAGER_H__

#include <Core/RefCounted.h>
#include <Core/Singleton.h>
#include <util/HashTable.h>
#include <Events/Events.h>
#include <DB/Dataset.h>
#include "ItemTpl.h"

// Item manager loads and keeps track of item templates, assists in saving & loading
// item-related stuff. Works like factory for CItemTpl type-specific derived classes.

namespace Data
{
	class CParams;
}

namespace Items
{
#define ItemMgr Items::CItemManager::Instance()

class CItemManager: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CItemManager);

private:

	static int				LargestItemStackID;

	CHashTable<CStrID, PItemTpl>	ItemTplRegistry;

	DB::PDataset						DSInv;
	DB::PDataset						DSEquip;
	nString								LevelInQuery;
	int									InitialVTRowCount;

	DECLARE_EVENT_HANDLER(OnSaveBefore, OnSaveBefore);
	DECLARE_EVENT_HANDLER(OnSaveAfter, OnSaveAfter);
	DECLARE_EVENT_HANDLER(OnLoadBefore, OnLoadBefore);
	DECLARE_EVENT_HANDLER(OnLoadAfter, OnLoadAfter);
	DECLARE_EVENT_HANDLER(OnGameDBClose, OnGameDBClose);

public:

	CItemManager();
	virtual ~CItemManager();

	PItemTpl		CreateItemTpl(CStrID ID, const CParams& Params); //???type from params?
	PItemTpl		GetItemTpl(CStrID ID);
	DB::CDataset*	GetInventoriesDataset() const { return DSInv.GetUnsafe(); }
	int				GetInventoriesRowCount() const { return InitialVTRowCount; }
	DB::CDataset*	GetEquipmentDataset() const { return DSEquip.GetUnsafe(); }
	int				NewItemStackID() const { return ++LargestItemStackID; }
};

}

#endif

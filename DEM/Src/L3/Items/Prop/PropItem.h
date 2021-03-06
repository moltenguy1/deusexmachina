#pragma once
#ifndef __DEM_L3_PROP_ITEM_H__
#define __DEM_L3_PROP_ITEM_H__

#include <Game/Property.h>
#include <Items/ItemStack.h>

// Item property contains item instance and allows to pick it up.

// Adds IAO actions:
// - Pick

namespace Properties
{

class CPropItem: public Game::CProperty
{
	DeclareRTTI;
	DeclareFactory(CPropItem);
	DeclarePropertyStorage;

protected:

	DECLARE_EVENT_HANDLER(OnSave, OnSave);
	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);
	DECLARE_EVENT_HANDLER(PickItem, OnPickItem);

public:

	Items::CItemStack Items;

	virtual void	Activate();
	virtual void	Deactivate();
	virtual void	GetAttributes(nArray<DB::CAttrID>& Attrs);
};

RegisterFactory(CPropItem);

}

#endif
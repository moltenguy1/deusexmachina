#pragma once
#ifndef __DEM_L3_PROP_EQUIPMENT_H__
#define __DEM_L3_PROP_EQUIPMENT_H__

#include <Items/Prop/PropInventory.h>

// Extends basic inventory with set of slots, accepting some volume/count of items of certain types
// and allowing characters to use these items and gain their effects.

namespace Properties
{

class CPropEquipment: public CPropInventory
{
	__DeclareClass(CPropEquipment);

protected:

	virtual void ExposeSI();
	virtual void Save();
	virtual void Load();

public:

	struct CSlot
	{
		CStrID		Type; //!!!need map: slot type => item types. Now 1:1.
		CItemStack*	pStack;
		WORD		Count;
		//WORD		MaxCount;

		CSlot(): pStack(NULL), Count(0) {}
	};

	nDictionary<CStrID, CSlot> Slots; //???to protected?

	CPropEquipment();
	//virtual ~CPropEquipment();

	//virtual void	Activate();
	//virtual void	Deactivate();

	bool Equip(CStrID Slot, CItemStack* pStack, WORD Count = 1); //???for 0 or -1 count as many as possible?
	void Unequip(CStrID SlotID);
};

__RegisterClassInFactory(CPropEquipment);

}

#endif
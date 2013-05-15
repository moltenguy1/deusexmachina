#pragma once
#ifndef __DEM_L2_GAME_PROPERTY_H__
#define __DEM_L2_GAME_PROPERTY_H__

// Properties are attached to game entities to add specific functionality or behaviors to the entity.

#include <Core/RefCounted.h>
#include <Data/StringID.h>
#include <Events/Events.h>
#include <util/HashTable.h>

#define __DeclarePropertyStorage \
	public: \
		static Game::CPropertyStorage* pStorage; \
		virtual Game::CPropertyStorage* GetStorage() const; \
	private:

#define __ImplementPropertyStorage(Class) \
	Game::CPropertyStorage* Class::pStorage = NULL; \
	Game::CPropertyStorage* Class::GetStorage() const { return pStorage; }

#define PROP_SUBSCRIBE_NEVENT(EventName, Class, Handler) \
	Sub_##EventName = GetEntity()->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler)
#define PROP_SUBSCRIBE_PEVENT(EventName, Class, Handler) \
	Sub_##EventName = GetEntity()->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler)

namespace Game
{
class CEntity;
typedef Ptr<class CProperty> PProperty;
typedef CHashTable<CStrID, PProperty> CPropertyStorage;

class CProperty: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	friend class CEntityManager;

	CEntity*	pEntity;
	bool		Active;

	void SetEntity(CEntity* pNewEntity);

	DECLARE_EVENT_HANDLER_VIRTUAL(OnEntityActivated, Activate);
	DECLARE_EVENT_HANDLER_VIRTUAL(OnEntityDeactivated, Deactivate);

public:

	CProperty(): Active(false), pEntity(NULL) {}
	virtual ~CProperty() = 0;

	virtual CPropertyStorage*	GetStorage() const { return NULL; }
	CEntity*					GetEntity() const { n_assert(pEntity); return pEntity; }
	bool						IsActive() const { return Active; }
};

}

#endif

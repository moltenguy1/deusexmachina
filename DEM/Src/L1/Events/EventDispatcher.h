#pragma once
#ifndef __DEM_L1_EVENT_DISPATCHER_H__
#define __DEM_L1_EVENT_DISPATCHER_H__

#include <Data/HashTable.h>
#include <Events/EventHandler.h>
#include <Events/Event.h>
#include <Events/EventNative.h>

// Event dispatcher receives fired events and dispatches them to subordinate dispatchers and subscribers.
// Subscribers can specify their priority, and higher priority subscriber receives event first.

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace Events
{

class CEventDispatcher: public Core::CRefCounted
{
protected:

	struct CEventNode
	{
		float			FireTime; //???store int or packed to int for CPU fast
		Ptr<CEventBase>	Event;
		CEventNode*		Next;
		CEventNode(): Next(NULL) {}
	};

	CEventNode* PendingEventsHead;
	CEventNode* PendingEventsTail; // to preserve events' fire order, insert to the end of the list
	CEventNode* EventsToAdd;

	// can use sorted array instead of list & implement subscription priority
	CHashTable<CEventID, PEventHandler> Subscriptions;

	DWORD	ScheduleEvent(CEventBase* Event, float RelTime);
	DWORD	DispatchEvent(const CEventBase& Event);

	// Event handler (to subscribe to other dispatcher and receive its events)
	bool	OnEvent(const CEventBase& Event) { return !!DispatchEvent(Event); }

public:

	CEventDispatcher();
	CEventDispatcher(int HashTableCapacity);
	virtual ~CEventDispatcher();

	bool					AddHandler(CEventID ID, PEventHandler Handler, PSub* pSub = NULL);

	// Returns handled counter (how much handlers have signed that they handled this event)
	DWORD					FireEvent(CStrID ID, Data::PParams Params = NULL, char Flags = 0, float RelTime = 0.f); /// non-native
	DWORD					FireEvent(CEventNative& Event, char Flags = -1, float RelTime = 0.f); /// native

	void					ProcessPendingEvents();

	//???leave 2 instead of 4 with CEventID first param?
	bool					Subscribe(CStrID ID, CEventCallback Callback, PSub* pSub = NULL, ushort Priority = Priority_Default);
	template<class T> bool	Subscribe(CStrID ID, T* Object, bool (T::*Callback)(const CEventBase&), PSub* pSub = NULL, ushort Priority = Priority_Default);
	bool					Subscribe(const CRTTI* RTTI, CEventCallback Callback, PSub* pSub = NULL, ushort Priority = Priority_Default);
	template<class T> bool	Subscribe(const CRTTI* RTTI, T* Object, bool (T::*Callback)(const CEventBase&), PSub* pSub = NULL, ushort Priority = Priority_Default);
	bool					Subscribe(CEventDispatcher& Listener, PSub* pSub = NULL, ushort Priority = Priority_Default);

	void					Unsubscribe(CEventID ID, CEventHandler* pHandler);
	void					UnsubscribeAll() { Subscriptions.Clear(); }
};

typedef Ptr<CEventDispatcher> PEventDispatcher;

inline CEventDispatcher::CEventDispatcher():
	PendingEventsHead(NULL),
	PendingEventsTail(NULL),
	EventsToAdd(NULL)
{
}
//---------------------------------------------------------------------

inline CEventDispatcher::CEventDispatcher(int HashTableCapacity):
	Subscriptions(HashTableCapacity),
	PendingEventsHead(NULL),
	PendingEventsTail(NULL),
	EventsToAdd(NULL)
{
}
//---------------------------------------------------------------------

inline DWORD CEventDispatcher::FireEvent(CStrID ID, Data::PParams Params, char Flags, float RelTime)
{
	//!!!event pools!
	Ptr<CEvent> Event = n_new(CEvent)(ID, Flags, Params); //EventSrv->ParamEvents.Allocate();
	return ScheduleEvent(Event, RelTime);
}
//---------------------------------------------------------------------

inline DWORD CEventDispatcher::FireEvent(CEventNative& Event, char Flags, float RelTime)
{
	if (Flags != -1) Event.Flags = Flags;
	return ScheduleEvent(&Event, RelTime);
}
//---------------------------------------------------------------------

inline bool CEventDispatcher::Subscribe(CStrID ID, CEventCallback Callback, PSub* pSub, ushort Priority)
{
	return AddHandler(ID, n_new(CEventHandlerCallback)(Callback, Priority), pSub);
}
//---------------------------------------------------------------------

template<class T>
inline bool CEventDispatcher::Subscribe(CStrID ID, T* Object, bool (T::*Callback)(const CEventBase&), PSub* pSub, ushort Priority)
{
	return AddHandler(ID, n_new(CEventHandlerMember<T>)(Object, Callback, Priority), pSub);
}
//---------------------------------------------------------------------

inline bool CEventDispatcher::Subscribe(const CRTTI* RTTI, CEventCallback Callback, PSub* pSub, ushort Priority)
{
	return AddHandler(RTTI, n_new(CEventHandlerCallback)(Callback, Priority), pSub);
}
//---------------------------------------------------------------------

template<class T>
inline bool CEventDispatcher::Subscribe(const CRTTI* RTTI, T* Object, bool (T::*Callback)(const CEventBase&), PSub* pSub, ushort Priority)
{
	return AddHandler(RTTI, n_new(CEventHandlerMember<T>)(Object, Callback, Priority), pSub);
}
//---------------------------------------------------------------------

inline bool CEventDispatcher::Subscribe(CEventDispatcher& Listener, PSub* pSub, ushort Priority)
{
	return AddHandler(NULL, n_new(CEventHandlerMember<CEventDispatcher>)(&Listener, &CEventDispatcher::OnEvent, Priority), pSub);
}
//---------------------------------------------------------------------

}

#endif
#include "EventDispatcher.h"

#include <Events/EventServer.h>
#include <Events/Subscription.h>
#include <Time/TimeServer.h>

namespace Events
{

CEventDispatcher::~CEventDispatcher()
{
	while (PendingEventsHead)
	{
		CEventNode* Next = PendingEventsHead->Next;
		EventSrv->EventNodes.Destroy(PendingEventsHead);
		PendingEventsHead = Next;
	}

	while (EventsToAdd)
	{
		CEventNode* Next = EventsToAdd->Next;
		EventSrv->EventNodes.Destroy(EventsToAdd);
		EventsToAdd = Next;
	}
}
//---------------------------------------------------------------------

bool CEventDispatcher::AddHandler(CEventID ID, PEventHandler Handler, PSub* pSub)
{
	PEventHandler& CurrSlot = Subscriptions.At(ID);
	if (CurrSlot.IsValid())
	{
		PEventHandler Prev, Curr = CurrSlot;
		while (Curr.IsValid() && Curr->GetPriority() > Handler->GetPriority())
		{
			Prev = Curr;
			Curr = Curr->Next;
		}

		if (Prev.IsValid()) Prev->Next = Handler;
		else CurrSlot = Handler;
		Handler->Next = Curr;
	}
	else CurrSlot = Handler;
	if (pSub) *pSub = n_new(CSubscription)(this, ID, Handler);
	OK;
}
//---------------------------------------------------------------------

DWORD CEventDispatcher::ScheduleEvent(CEventBase* Event, float RelTime)
{
	n_assert(Event);

 	if (RelTime > 0.f) Event->Flags |= EV_ASYNC;

	if (Event->Flags & EV_ASYNC)
	{
		CEventNode* New = EventSrv->EventNodes.Construct();
		n_assert2(New, "Nervous system of the engine was paralyzed! Can't allocate event node");
		New->Event = Event;
		New->FireTime = (float)TimeSrv->GetTime() + RelTime;
		if (PendingEventsTail)
		{
			n_assert(PendingEventsHead);

			if (New->FireTime >= PendingEventsTail->FireTime)
			{
				PendingEventsTail->Next = New;
				PendingEventsTail = New;
			}
			else
			{
				if (PendingEventsHead == PendingEventsTail)
				{
					PendingEventsHead = New;
					PendingEventsHead->Next = PendingEventsTail;
				}
				else if (EventsToAdd)
				{
					CEventNode* Curr = EventsToAdd;
					CEventNode* Next = EventsToAdd->Next;
					while (Curr)
					{
						if (New->FireTime >= Curr->FireTime)
						{
							Curr->Next = New;
							New->Next = Next;
							break;
						}
						Curr = Next;
						Next = Next->Next;
					}
				}
				else EventsToAdd = New;
			}
		}
		else
		{
			n_assert(!PendingEventsHead);
			PendingEventsHead =
			PendingEventsTail = New;
		}

		return (DWORD)(-1); // Can't count handle facts for async events
	}
	else return DispatchEvent(*Event);
}
//---------------------------------------------------------------------

DWORD CEventDispatcher::DispatchEvent(const CEventBase& Event)
{
	DWORD HandledCounter = 0;
	PEventHandler Sub;

	// Look for subscriptions to this event
	if (Subscriptions.Get(Event.GetID(), Sub)) do
	{
		if (Sub->operator ()(Event))
		{
			++HandledCounter;
			if (Event.Flags & EV_TERM_ON_HANDLED) return HandledCounter;
		}
		Sub = Sub->Next;
	}
	while (Sub.IsValid());

	// Look for subscriptions to any event
	if (Subscriptions.Get(NULL, Sub)) do
	{
		if (Sub->operator ()(Event))
		{
			++HandledCounter;
			if (Event.Flags & EV_TERM_ON_HANDLED) return HandledCounter;
		}
		Sub = Sub->Next;
	}
	while (Sub.IsValid());

	return HandledCounter;
}
//---------------------------------------------------------------------

void CEventDispatcher::ProcessPendingEvents()
{
	if (EventsToAdd)
	{
		n_assert(PendingEventsHead);

		CEventNode* Curr = PendingEventsHead;
		CEventNode* Next = PendingEventsHead->Next;
		while (Curr)
		{
			if (EventsToAdd->FireTime >= Curr->FireTime)
			{
				Curr->Next = EventsToAdd;
				while (EventsToAdd->Next && EventsToAdd->Next->FireTime <= Next->FireTime)
					EventsToAdd = EventsToAdd->Next;
				if (!EventsToAdd->Next)
				{
					EventsToAdd->Next = Next;
					EventsToAdd = NULL;
					break;
				}
				CEventNode* NextToAdd = EventsToAdd->Next;
				EventsToAdd->Next = Next;
				EventsToAdd = NextToAdd;
			}
			Curr = Next;
			n_assert(Next); // will always be because insertions to 1-elm list are done immediately on queuing above
			Next = Next->Next;
		}
	}

	float CurrTime = (float)TimeSrv->GetTime();

	while (PendingEventsHead && PendingEventsHead->FireTime <= CurrTime)
	{
		n_assert(PendingEventsHead->Event);
		DispatchEvent(*PendingEventsHead->Event);
		CEventNode* Next = PendingEventsHead->Next;
		EventSrv->EventNodes.Destroy(PendingEventsHead);
		PendingEventsHead = Next;
	}
	if (!PendingEventsHead) PendingEventsTail = NULL;
}
//---------------------------------------------------------------------

void CEventDispatcher::Unsubscribe(CEventID ID, CEventHandler* pHandler)
{
	n_assert_dbg(pHandler);

	PEventHandler* pCurrSlot = Subscriptions.Get(ID);
	if (pCurrSlot)
	{
		PEventHandler Prev, Curr = *pCurrSlot;
		do
		{
			if (Curr.GetUnsafe() == pHandler)
			{
				if (Prev.IsValid()) Prev->Next = pHandler->Next;
				else
				{
					if (pHandler->Next.IsValid()) (*pCurrSlot) = pHandler->Next;
					else Subscriptions.Remove(ID); //???optimize duplicate search? use CIterator?
				}
				return;
			}
			Prev = Curr;
			Curr = Curr->Next;
		}
		while (Curr.IsValid());
	}

	n_error("Subscription on '%s' not found, mb double unsubscription", ID.ID);
}
//---------------------------------------------------------------------

}

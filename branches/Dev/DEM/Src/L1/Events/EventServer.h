#pragma once
#ifndef __DEM_L1_EVENT_MANAGER_H__
#define __DEM_L1_EVENT_MANAGER_H__

#include "EventDispatcher.h"
#include "EventBase.h"
#include <Data/Pool.h>
#include <Core/Singleton.h>

// Event manager is a central coordination point for the event processing. It works as:
// - Factory/Cache, by producing event, subscription etc nodes for dispatchers' internal usage
// - Global event dispatcher if you want to send application-scope events

namespace Data
{
	class CParams;
}

namespace Events
{
#define EventSrv Events::CEventServer::Instance()

class CEventServer: public CEventDispatcher
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CEventServer);

public:

	// For internal usage
	CPool<CEventNode>		EventNodes;

	//!!!cache pool of free events, get from here if can
	//or store cache pools in CEventServer

	CEventServer();
	virtual ~CEventServer();

	//!!!use pool inside!
	//Ptr<CEventNative>			CreateNativeEvent(const Core::CRTTI* RTTI);
	//template<class T> Ptr<T>	CreateNativeEvent();
};

}

#endif

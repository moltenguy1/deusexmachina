#include "AIServer.h"

#include <AI/Planning/ActionTpl.h>
#include <Data/Params.h>
#include <IO/IOServer.h>
#include <Events/EventManager.h>
#include <DetourNavMeshQuery.h>
#include <DetourObstacleAvoidance.h>

namespace AI
{
__ImplementClassNoFactory(AI::CAIServer, Core::CRefCounted);
__ImplementSingleton(AI::CAIServer);

CAIServer::CAIServer()
{
	__ConstructSingleton;
	IOSrv->SetAssign("actors", "game:ai/actors"); //!!!unwind!
	IOSrv->SetAssign("aihints", "game:ai/hints");
	IOSrv->SetAssign("smarts", "game:ai/smarts");

	dtObstacleAvoidanceParams* pOAParams = n_new(dtObstacleAvoidanceParams);
	pOAParams->velBias = 0.4f;
	pOAParams->weightDesVel = 2.0f;
	pOAParams->weightCurVel = 0.75f;
	pOAParams->weightSide = 0.75f;
	pOAParams->weightToi = 2.5f;
	pOAParams->horizTime = 2.5f;
	pOAParams->gridSize = 33;
	pOAParams->adaptiveDivs = 7;
	pOAParams->adaptiveRings = 2;
	pOAParams->adaptiveDepth = 5;
	ObstacleAvoidanceParams.Add(CStrID::Empty, pOAParams);

	dtQueryFilter* pNavFilter = n_new(dtQueryFilter);
	pNavFilter->setExcludeFlags(NAV_FLAG_LOCKED);
	NavQueryFilters.Add(CStrID::Empty, pNavFilter);

	for (int i = 0; i < DEM_THREAD_COUNT; ++i)
		n_assert(PathQueues[i].Init(MAX_NAV_PATH)); //???reinit on each level loading?
}
//---------------------------------------------------------------------

CAIServer::~CAIServer()
{
	for (int i = 0; i < NavQueryFilters.GetCount(); ++i)
		n_delete(NavQueryFilters.ValueAtIndex(i));

	for (int i = 0; i < ObstacleAvoidanceParams.GetCount(); ++i)
		n_delete(ObstacleAvoidanceParams.ValueAtIndex(i));

	__DestructSingleton;
}
//---------------------------------------------------------------------

void CAIServer::Trigger()
{
	for (int i = 0; i < DEM_THREAD_COUNT; ++i)
		PathQueues[i].Update(100);
}
//---------------------------------------------------------------------

void CAIServer::AddSmartObjActionTpl(CStrID ID, const CParams& Desc)
{
	if (!SOActTpls.Contains(ID)) SOActTpls.Add(ID, CSmartObjActionTpl(Desc));
}
//---------------------------------------------------------------------

const CSmartObjActionTpl* CAIServer::GetSmartObjActionTpl(CStrID ID) const
{
	int Idx = SOActTpls.FindIndex(ID);
	return (Idx != INVALID_INDEX) ? &SOActTpls.ValueAtIndex(Idx) : NULL;
}
//---------------------------------------------------------------------

} //namespace AI
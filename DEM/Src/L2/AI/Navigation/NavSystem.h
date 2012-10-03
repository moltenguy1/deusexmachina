#pragma once
#ifndef __DEM_L2_AI_NAV_SYSTEM_H__
#define __DEM_L2_AI_NAV_SYSTEM_H__

#include <StdDEM.h>
#include <Core/Ptr.h>
#include <AI/Navigation/PathEdge.h>
#include <AI/ActorFwd.h>
#include <util/ndictionary.h>
#include <util/nstring.h>
#include <DetourPathCorridor.h>

// Navigation system is responsible for path planning, destination tracking, high-level spatial status tracking.
// Low-level movement, facing etc are performed in the MotorSystem.

//!!!DT_MAX_AREAS as invalid for GetAreaUnderTheFeet!

namespace Data
{
	typedef Ptr<class CParams> PParams;
};

class dtLocalBoundary;
class dtObstacleAvoidanceQuery;

namespace AI
{

enum ENavStatus
{
	AINav_Invalid,		// Current actor's position is invalid for navigation
	AINav_DestSet,		// Destination is set and path is required, trying to find path fast and sync
	AINav_Planning,		// Fast sync pathfinding was not succeed, performing full async pathfinding
	AINav_Following,	// Actor has valid path and follows it
	AINav_Done,			// Actor is at the destination, NavSystem is idle
	AINav_Failed		// Path planning or following failed, NavSystem is idle
};

class CPathRequestQueue;

class CNavSystem
{
protected:

	CActor*						pActor;

	const dtQueryFilter*		pNavFilter;
	dtPathCorridor				Corridor;
	dtLocalBoundary*			pBoundary;

	vector3						DestPoint;
	dtPolyRef					DestRef;

	//nArray<CPathEdge>			Path;

	dtPolyRef					OffMeshRef;
	vector3						OffMeshPoint;
	float						OffMeshRadius;
	bool						TraversingOffMesh;

	float						ReplanTime;
	float						TopologyOptTime;
	CPathRequestQueue*			pProcessingQueue;
	DWORD						PathRequestID;

	//???personal or template?
	nDictionary<int, CStrID>	EdgeTypeToAction;

	//!!!Path info cache

	CStrID	GetPolyAction(const dtNavMesh* pNavMesh, dtPolyRef Ref);

public:

	CNavSystem(CActor* Actor);

	void			Init(const Data::CParams* Params);
	void			Term();
	void			Update(float FrameTime);
	void			Reset();

	void			UpdatePosition();
	void			EndEdgeTraversal();
	bool			GetPathEdges(nArray<CPathEdge>& OutPath, int MaxSize = MAX_SDWORD);
	void			GetObstacles(float Range, dtObstacleAvoidanceQuery& Query);
	//bool			GetRandomValidLocation(float Range, vector3& Location);

	bool			IsTraversingOffMesh() const { return TraversingOffMesh; }
	void			SetDestPoint(const vector3& Dest);
	const vector3&	GetDestPoint() const { return DestPoint; }
};

}

#endif
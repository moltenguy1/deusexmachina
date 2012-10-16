#include "CIDEApp.h"

#include "AppStateEditor.h"
#include <App/Environment.h>
#include <App/CSharpUIEventHandler.h>
#include <Game/Mgr/EntityManager.h>
#include <Game/Mgr/StaticEnvManager.h>
#include <Game/Mgr/FocusManager.h>
#include <Loading/EntityFactory.h>
#include <Loading/EntityLoader.h>
#include <SI/SI_L1.h>
#include <SI/SI_L2.h>
#include <SI/SI_L3.h>
#include <Prop/PropEditorCamera.h>
#include <Input/Prop/PropInput.h>
#include <Game/Mgr/EnvQueryManager.h>
#include <Physics/Prop/PropTransformable.h>
#include <Physics/Prop/PropAbstractPhysics.h>
#include <Physics/CharEntity.h>
#include <Physics/Composite.h>
#include <Data/Streams/FileStream.h>
#include <gfx2/ngfxserver2.h>

namespace Attr
{
	DeclareAttr(Name);
	DeclareAttr(Center);
	DeclareAttr(Extents);
	DeclareAttr(NavMesh);
}

namespace App
{
__ImplementSingleton(App::CCIDEApp);

using namespace Properties;

CCIDEApp::CCIDEApp():
	ParentHwnd(NULL),
	pUIEventHandler(NULL),
	pNavMeshBuilder(NULL),
	DenyEntityAboveGround(false),
	DenyEntityBelowGround(false)
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CCIDEApp::~CCIDEApp()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CCIDEApp::Open()
{
	nString WindowTitle = GetVendorName() + " - " + GetAppName() + " - " + GetAppVersion();

	AppEnv->SetVendorName(GetVendorName());
	AppEnv->SetAppName(GetAppName());
	AppEnv->SetAppVersion(GetAppVersion());
	AppEnv->SetDisplayMode(CDisplayMode(0, 0, 800, 600, false));
	AppEnv->SetWindowTitle(WindowTitle.Get());
	AppEnv->SetWindowIcon("Icon");

	//!!!SET FULLSCREEN / WINDOWED! //???to display mode?

	if (!AppEnv->InitCore())
	{
		AppEnv->ReleaseCore();
		FAIL;
	}

	//!!!TO HRD SETTINGS!
	nString Home = DataSrv->ManglePath("home:");
	nString Proj = DataSrv->ManglePath("proj:");
	nString Data = Proj + "/Content/Project";
	nString Export = Proj + "/Content/export";
	nString Src = Proj + "/Content/Src";
	DataSrv->SetAssign("data", Data);
	DataSrv->SetAssign("export", Export);
	DataSrv->SetAssign("src", Src);
	DataSrv->SetAssign("renderpath", Home + "/Shaders/");
	DataSrv->SetAssign("scripts", Home + "/Scripts/");
	DataSrv->SetAssign("physics", Export + "/physics/");
	DataSrv->SetAssign("meshes", Export + "/meshes/");
	DataSrv->SetAssign("textures", Export + "/textures/");
	DataSrv->SetAssign("anims", Export + "/anims/");
	DataSrv->SetAssign("gfxlib", Export + "/gfxlib/");
	DataSrv->SetAssign("db", Export + "/db/");
	DataSrv->SetAssign("game", Export + "/game/");
	DataSrv->SetAssign("sound", Export + "/audio/");
	DataSrv->SetAssign("cegui", Export + "/cegui/");

	CoreSrv->SetGlobal("parent_hwnd", (int)ParentHwnd);

	if (!AppEnv->InitEngine())
	{
		AppEnv->ReleaseEngine();
		AppEnv->ReleaseCore();
		FAIL;
	}

	DbgSrv->AllowUI(false);

	nGfxServer2::Instance()->SetCursorVisibility(nGfxServer2::System);

	SI::RegisterGlobals();
	SI::RegisterEventManager();
	SI::RegisterEntityManager();

	InputSrv->SetContextLayout(CStrID("Debug"), CStrID("Debug"));
	InputSrv->SetContextLayout(CStrID("Game"), CStrID("Game"));
	InputSrv->SetContextLayout(CStrID("Editor"), CStrID("Editor"));

	InputSrv->EnableContext(CStrID("Debug"));

	n_printf("Setup input - OK\n");

	// Editor must load all static entities separately, so we don't enable CEnvironmentLoader
	EntityFct->SetDefaultLoader(Loading::CEntityLoader::Create());

	if (!AppEnv->InitGameSystem())
	{
		AppEnv->ReleaseGameSystem();
		AppEnv->ReleaseEngine();
		AppEnv->ReleaseCore();
		FAIL;
	}

	// Init gameplay
	QuestSystem = Story::CQuestSystem::Create();
	DlgSystem = Story::CDlgSystem::Create(); //???init into Game::Server as manager?
	ItemManager = Items::CItemManager::Create(); //???init into Game::Server as manager?

	SI::RegisterQuestSystem();

	FSM.AddStateHandler(n_new(CAppStateEditor(CStrID("Editor"))));
	FSM.Init(CStrID("Editor"));

	DB::PValueTable CameraTable = DB::CValueTable::Create();
	CameraTable->AddColumn(Attr::GUID);
	CameraTable->AddColumn(Attr::FieldOfView);
	CameraTable->AddColumn(Attr::Transform);

	EditorCamera = EntityFct->CreateTmpEntity(CStrID("__EditorCamera"), CStrID("Dummy"), CameraTable, CameraTable->AddRow());
	EntityFct->AttachProperty<Properties::CPropEditorCamera>(*EditorCamera);
	EntityFct->AttachProperty<Properties::CPropInput>(*EditorCamera);
	EntityFct->AttachProperty<Properties::CPropTransformable>(*EditorCamera);
	matrix44 Tfm;
	//!!!bad hardcode!
	Tfm.lookatRh(vector3(0.f, -3.f, -2.f), vector3(0.f, 1.f, 0.f));
	Tfm.translate(vector3(300.f, 145.f, 300.f));
	EditorCamera->Set<matrix44>(Attr::Transform, Tfm);

	pUIEventHandler = n_new(CCSharpUIEventHandler);
	pNavMeshBuilder = n_new(CNavMeshBuilder);

	DB::PTable Tbl;
	int TblIdx = LoaderSrv->GetGameDB()->FindTableIndex("Levels");
	if (TblIdx == INVALID_INDEX)
	{
		Tbl = DB::CTable::Create();
		Tbl->SetName("Levels");
		Tbl->AddColumn(DB::CColumn(Attr::GUID, DB::CColumn::Primary));
		Tbl->AddColumn(Attr::Name);
		Tbl->AddColumn(Attr::Center);
		Tbl->AddColumn(Attr::Extents);
		Tbl->AddColumn(Attr::NavMesh);
		LoaderSrv->GetGameDB()->AddTable(Tbl);
	}
	else Tbl = LoaderSrv->GetGameDB()->GetTable(TblIdx);
	Levels = Tbl->CreateDataset();
	Levels->PerformQuery();

	OK;
}
//---------------------------------------------------------------------

void CCIDEApp::Close()
{
	Levels = NULL;

	n_delete(pNavMeshBuilder);
	pNavMeshBuilder = NULL;
	n_delete(pUIEventHandler);
	pUIEventHandler = NULL;

	ClearSelectedEntities();
	CurrentEntity = NULL;
	EditorCamera = NULL;

	AttrDescs = NULL;

	FSM.Clear();

	DlgSystem = NULL;
	QuestSystem = NULL;
	ItemManager = NULL;

	AppEnv->ReleaseGameSystem();
	AppEnv->ReleaseEngine();
	AppEnv->ReleaseCore();
}
//---------------------------------------------------------------------

bool CCIDEApp::SetEditorTool(LPCSTR Name)
{
	CAppStateEditor* pState = (CAppStateEditor*)FSM.FindStateHandlerByID(CStrID("Editor"));
	return pState && pState->SetTool(Name);
}
//---------------------------------------------------------------------

bool CCIDEApp::SelectEntity(Game::PEntity Entity)
{
	if (!Entity.isvalid()) FAIL;
	if (Entity.get_unsafe() != SelectedEntity.get_unsafe())
		SelectedEntity = Entity;
	OK;
}
//---------------------------------------------------------------------

void CCIDEApp::ClearSelectedEntities()
{
	SelectedEntity = NULL;
}
//---------------------------------------------------------------------

void CCIDEApp::ApplyGroundConstraints(const Game::CEntity& Entity, vector3& Position)
{
	if (!DenyEntityAboveGround && !DenyEntityBelowGround) return;

	int SelfPhysicsID;
	CPropAbstractPhysics* pPhysProp = Entity.FindProperty<CPropAbstractPhysics>();

	float LocalMinY = 0.f;
	if (pPhysProp)
	{
		Physics::CEntity* pPhysEnt = pPhysProp->GetPhysicsEntity();
		if (pPhysEnt)
		{
			SelfPhysicsID = pPhysEnt->GetUniqueID();
			
			bbox3 AABB;
			pPhysEnt->GetComposite()->GetAABB(AABB);

			//!!!THIS SHOULDN'T BE THERE! Tfm must be adjusted inside entity that fixes it.
			// I.e. AABB must be translated in GetAABB or smth.
			float PhysEntY = pPhysEnt->GetTransform().pos_component().y;
			float AABBPosY = AABB.center().y;

			LocalMinY -= (AABB.extents().y - AABBPosY + PhysEntY);

			//!!!tmp hack, need more general code!
			if (pPhysEnt->IsA(Physics::CCharEntity::RTTI))
				LocalMinY -= ((Physics::CCharEntity*)pPhysEnt)->Hover;
		}
		else SelfPhysicsID = -1;
	}
	else SelfPhysicsID = -1;

	CEnvInfo Info;
	EnvQueryMgr->GetEnvInfoAt(vector3(Position.x, Position.y + 500.f, Position.z), Info, 1000.f, SelfPhysicsID);
	
	float MinY = Position.y + LocalMinY;
	if ((DenyEntityAboveGround && MinY > Info.WorldHeight) ||
		(DenyEntityBelowGround && MinY < Info.WorldHeight))
		Position.y = Info.WorldHeight - LocalMinY;
}
//---------------------------------------------------------------------

//???!!!CStrID!
bool CCIDEApp::LoadLevel(const nString& ID)
{
	UnloadLevel(true);
	if (!LoaderSrv->LoadLevel(ID)) FAIL;

	Data::CFileStream File;
	if (File.Open("src:Levels/" + ID + "/Info.lvl", Data::SAM_READ))
	{
		int Count = File.Get<int>();
		if (Count) File.Read(CurrLevel.ConvexVolumes.Reserve(Count), Count * sizeof(CConvexVolume));
		Count = File.Get<int>();
		if (Count) File.Read(CurrLevel.OffmeshConnections.Reserve(Count), Count * sizeof(COffmeshConnection));
		File.Close();
	}
	CurrLevel.ID = ID;
	CurrLevel.ConvexChanged = true;
	CurrLevel.OffmeshChanged = true;
	InvalidateNavGeometry();

	EntityMgr->AttachEntity(EditorCamera);
	FocusMgr->SetFocusEntity(EditorCamera);

	OK;
}
//---------------------------------------------------------------------

void CCIDEApp::UnloadLevel(bool SaveChanges)
{
	ClearSelectedEntities();

	if (SaveChanges && CurrLevel.ID.Length() > 0)
	{
		DataSrv->CreateDirectory("src:Levels/" + CurrLevel.ID);

		Data::CFileStream File;
		if (File.Open("src:Levels/" + CurrLevel.ID + "/Info.lvl", Data::SAM_WRITE))
		{
			int Count = CurrLevel.ConvexVolumes.Size();
			File.Put<int>(Count);
			if (Count) File.Write(CurrLevel.ConvexVolumes.Begin(), Count * sizeof(CConvexVolume));
			Count = CurrLevel.OffmeshConnections.Size();
			File.Put<int>(Count);
			if (Count) File.Write(CurrLevel.OffmeshConnections.Begin(), Count * sizeof(COffmeshConnection));
			File.Close();
		}

		//!!!save navigation mesh, ALSO save on build (export)
		//don't forget to save level's ref to nav mesh resource
		
		LoaderSrv->CommitChangesToDB();
	}

	CurrLevel.ID = NULL;
	CurrLevel.ConvexVolumes.Clear();
	CurrLevel.OffmeshConnections.Clear();

	LoaderSrv->UnloadLevel();
}
//---------------------------------------------------------------------

int CCIDEApp::GetLevelCount() const
{
	return Levels->GetRowCount();
}
//---------------------------------------------------------------------

bool CCIDEApp::BuildNavMesh(const char* pRsrcName, float AgentRadius, float AgentHeight, float MaxClimb)
{
	if (CurrLevel.ID.Length() == 0) FAIL;

	//m_cellSize = 0.3f;
	//m_cellHeight = 0.2f;
	//m_agentHeight = 2.0f;
	//m_agentRadius = 0.6f;
	//m_agentMaxClimb = 0.9f;
	//m_agentMaxSlope = 45.0f;
	//m_regionMinSize = 8;
	//m_regionMergeSize = 20;
	//m_monotonePartitioning = false;
	//m_edgeMaxLen = 12.0f;
	//m_edgeMaxError = 1.3f;
	//m_vertsPerPoly = 6.0f;
	//m_detailSampleDist = 6.0f;
	//m_detailSampleMaxError = 1.0f;

	rcConfig Cfg;
	memset(&Cfg, 0, sizeof(Cfg));

	//!!!need to calc from geometry selected!
	const bbox3& Box = LoaderSrv->GetCurrentLevelBox();
	rcVcopy(Cfg.bmin, Box.vmin.v);
	rcVcopy(Cfg.bmax, Box.vmax.v);

	float BoxSizeX = Box.vmax.x - Box.vmin.x;
	float BoxSizeZ = Box.vmax.z - Box.vmin.z;
	float BoxSizeMax = n_max(BoxSizeX, BoxSizeZ);
	Cfg.cs = BoxSizeMax * 0.00029f; // nearly one over 3450.f, empirically detected
	Cfg.ch = 0.2f;
	Cfg.walkableSlopeAngle = 45.0f;
	Cfg.maxEdgeLen = (int)(12.0f / Cfg.cs);
	Cfg.maxSimplificationError = 1.3f;

	// empirically detected, one over 133.(3)f
	int Factor = (int)n_floor(BoxSizeMax * 0.0075f + 0.5f); // 0.5f to round 1.5 to 2
	Cfg.minRegionArea = (int)rcSqr(Factor);					// Note: area = size*size

	Cfg.mergeRegionArea = (int)rcSqr(20);				// Note: area = size*size
	Cfg.maxVertsPerPoly = DT_VERTS_PER_POLYGON;
	Cfg.detailSampleDist = 6.0f < 0.9f ? 0.f : Cfg.cs * 6.0f;
	Cfg.detailSampleMaxError = Cfg.ch * 1.0f;

	n_printf("NavMesh building started\n");

	//!!! || bbox changed || settings like cs & ch changed!
	bool ResetGeometry = CurrLevel.NavGeometryChanged;

	if (ResetGeometry)
	{
		if (!pNavMeshBuilder->Init(Cfg, MaxClimb)) FAIL;

		n_printf("pNavMeshBuilder->Init OK\n");

		const nArray<Game::PEntity>& Ents = EntityMgr->GetEntities();
		for (int i = 0; i < Ents.Size(); ++i)
		{
			Game::CEntity& Ent = *Ents[i];
			if (StaticEnvMgr->IsEntityStatic(Ent))
				pNavMeshBuilder->AddGeometry(Ent);
		}

		CurrLevel.NavGeometryChanged = false;

		n_printf("NavMesh geom. added\n");
	}

	// Now only one R+H at a time is allowed, later need a set of pCompactHFs inside an NMB
	bool ReprepareGeometry =
		ResetGeometry || AgentRadius != pNavMeshBuilder->GetRadius() || AgentHeight != pNavMeshBuilder->GetHeight();

	if (ReprepareGeometry && !pNavMeshBuilder->PrepareGeometry(AgentRadius, AgentHeight)) FAIL;

	n_printf("pNavMeshBuilder->PrepareGeometry OK\n");

	bool RebuildMesh = ReprepareGeometry || CurrLevel.ConvexChanged;

	if (RebuildMesh)
	{
		if (!ReprepareGeometry) pNavMeshBuilder->ResetAllArea();

		for (int i = 0; i < CurrLevel.ConvexVolumes.Size(); ++i)
			pNavMeshBuilder->ApplyConvexVolumeArea(CurrLevel.ConvexVolumes[i]);
		CurrLevel.ConvexChanged = false;
	}

	if (RebuildMesh)
	{
		if (!pNavMeshBuilder->BuildNavMesh()) FAIL;
		if (!pNavMeshBuilder->BuildDetailMesh()) FAIL;
	}

	bool DataChanged = RebuildMesh | CurrLevel.OffmeshChanged;

	if (CurrLevel.OffmeshChanged)
	{
		pNavMeshBuilder->ClearOffmeshConnections();
		for (int i = 0; i < CurrLevel.OffmeshConnections.Size(); ++i)
			pNavMeshBuilder->AddOffmeshConnection(CurrLevel.OffmeshConnections[i]);
		CurrLevel.OffmeshChanged = false;
	}

	if (DataChanged)
	{
		// Can't use CBuffer due to dtCreateNavMeshData implementation
		uchar* pData;
		int Size;
		if (!pNavMeshBuilder->GetNavMeshData(pData, Size)) FAIL;

		//???free pData after writing?

		n_printf("pNavMeshBuilder->GetNavMeshData OK\n");

		// Detect poly list for each volume
		for (int i = 0; i < CurrLevel.ConvexVolumes.Size(); ++i)
		{
			CConvexVolume& Vol = CurrLevel.ConvexVolumes[i];
			if (Vol.Area = NAV_AREA_NAMED)
			{
				Data::CBuffer Buffer;
				pNavMeshBuilder->GetRegionData(Vol, Buffer);
			}
		}

		nString Path;
		Path.Format("export:Nav/%s.nm", pRsrcName);
		n_printf("NavMesh file path: %s\n", DataSrv->ManglePath(Path).Get());
		if (DataSrv->CreateDirectory(Path.ExtractDirName()))
		{
			Data::CFileStream File;
			if (File.Open(Path.Get(), Data::SAM_WRITE))
			{
				//!!!Save data!
				File.Write(pData, Size);
				File.Close();
				n_printf("NavMesh saved\n");
			}
		}

		dtFree(pData);
	}

	OK;
}
//---------------------------------------------------------------------

void CCIDEApp::InvalidateNavGeometry()
{
	CurrLevel.NavGeometryChanged = true;
	pNavMeshBuilder->Cleanup();
}
//---------------------------------------------------------------------

} // namespace App
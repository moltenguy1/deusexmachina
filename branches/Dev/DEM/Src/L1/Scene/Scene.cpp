#include "Scene.h"

#include <Scene/SceneServer.h>
#include <Events/EventManager.h>

namespace Scene
{

void CScene::Init(const CAABB& Bounds, DWORD SPSHierarchyDepth)
{
	RootNode = n_new(CSceneNode(*this, CStrID::Empty));

	//???is always needed?
	//???create from L2 camera manager? scene hasn't to have camera, it is an user's responsibility
	//but LODs use camera!
	CSceneNode* CameraNode = RootNode->CreateChild(CStrID("_DefaultCamera"));
	DefaultCamera = n_new(CCamera);
	CameraNode->AddAttr(*DefaultCamera);
	MainCamera = DefaultCamera;

	MainCamera->SetWidth((float)RenderSrv->GetBackBufferWidth());
	MainCamera->SetHeight((float)RenderSrv->GetBackBufferHeight());

	SceneBBox = Bounds;
	vector3 Center = SceneBBox.Center();
	vector3 Size = SceneBBox.Size();
	SPS.Build(Center.x, Center.z, Size.x, Size.z, (uchar)SPSHierarchyDepth);

	//!!!could subscribe only when AutoAdjustCameraAspect is set to true!
	SUBSCRIBE_PEVENT(OnRenderDeviceReset, CScene, OnRenderDeviceReset);
}
//---------------------------------------------------------------------

void CScene::Activate()
{
}
//---------------------------------------------------------------------

void CScene::Deactivate()
{
}
//---------------------------------------------------------------------

void CScene::Clear()
{
	UNSUBSCRIBE_EVENT(OnRenderDeviceReset);
	RootNode = NULL;
}
//---------------------------------------------------------------------

void CScene::SetMainCamera(CCamera* pNewCamera)
{
	MainCamera = pNewCamera ? pNewCamera : DefaultCamera;
	if (AutoAdjustCameraAspect)
	{
		MainCamera->SetWidth((float)RenderSrv->GetBackBufferWidth());
		MainCamera->SetHeight((float)RenderSrv->GetBackBufferHeight());
	}
}
//---------------------------------------------------------------------

bool CScene::Render(PCamera Camera, Render::CFrameShader& FrameShader) //, CStrID FrameShaderID)
{
	if (!Camera.IsValid()) Camera = MainCamera;
	if (!Camera.IsValid()) Camera = DefaultCamera;
	if (!Camera.IsValid())
	{
		ClearVisibleLists();
		FAIL;
	}

	//!!!FrameShader OPTIONS!
	bool FrameShaderUsesLights = true;
	//!!!filters (ShadowCasters etc)!

	const matrix44& ViewProj = Camera->GetViewProjMatrix();

	//???need base class for models, particle systems, terrain patches etc?
	// so gather there CRenderObject* and CLight* arrays and feed renderers

	//!!!filter flags (from frame shader - or-sum of pass flags, each pass will check requirements inside itself)
	CArray<CLight*>* pVisibleLights = FrameShaderUsesLights ? &VisibleLights : NULL;
	SPSCollectVisibleObjects(SPS.GetRootNode(), ViewProj, &VisibleObjects, pVisibleLights);

	RenderSrv->SetAmbientLight(AmbientLight);
	RenderSrv->SetCameraPosition(Camera->GetPosition());
	RenderSrv->SetViewProjection(ViewProj);

	FrameShader.Render(&VisibleObjects, pVisibleLights);

// Dependent cameras:
	// Some shapes may request textures that are RTs of specific cameras
	// These textures must be rendered before shapes are rendered
	// Good way is to collect all cameras and recurse, filling required textures
	// Pass may disable recursing into cameras to prevent infinite recursion
	// Generally, only FrameBuffer pass should recurse
	// Camera shouldn't render its texture more than once per frame (in won't change)!
	//???as "RenderDependentCameras" flag in Camera/Pass? maybe even filter by dependent camera type
	// Collect - check all meshes to be rendered, check all their textures, select textures rendered from camera RTs
	// Non-mesh dependent textures (shadows etc) must be rendered in one of the previous passes

// Somewhere at here scene ends and renderer begins:

	// Shapes are sorted by shader, by distance (None, FtB or BtF, depending on shader requirements),
	// by geometry (for the instancing), may be by lights that affect them
	// For the front-to-back sorting, can sort once on first request, always FtB, and when BtF is needed,
	// iterate through the array from the end reversely

	// For each shader (batch):
	// Shader is set
	// Light params are updated, if it is light pass
	// Shader params are updated
	// Shapes are rendered, instanced, when possible

//!!!NOTE: meshes visible from the light's camera are the meshes in light's range!
// can avoid collecting visible meshes on shadow pass
// can render non-shadow-receiving objects without shadow mapping (another technique)
// can render to SM only casters with extruded shadow box visible from main camera

// Rendering must be like this:
	// - begin frame shader
	// - Renderer: apply frame shader constant render states
	// - Renderer: set View and Projection
	// - determine visible meshes [and lights, if lighting is enabled ?in one of passes?]
	// - for each pass, render scene pass, occlusion pass, shadow pass (for shadow-casting lights) or posteffect pass
	// - end frame shader
	//
	// Scene pass:
	// - begin pass
	// - if pass renders dependent textures
	//   - collect them and their cameras (check if already up-to-date this frame)
	//   - if recurse to rendering call with another camera and frame shader
	//   - now our textures from cameras are ready to use
	// - Renderer: set RT
	// - Renderer: optionally clear RT
	// - Renderer: apply pass shader (divide this pass to some phases/batches like opaque, atest, alpha etc inside, mb before a renderer...)
	// - Renderer: set pass technique
	// - Renderer: pass meshes [and lights]
	// - Renderer: render to RT
	//    (link meshes and lights(here?), sort meshes, batch instances, select lighting code,
	//     set shared state of instance sets)
	// - end pass
	//
	// Occlusion pass:
	// - begin pass
	// - issue occlusion query for AABBs of all visible meshes
	// - remove occluded shapes [and lights] from visible shape [and light] array[s]
	// - end pass
	//
	// Shadow pass:
	// - begin pass
	// - select shadow-casting light(s)
	// - instead of collecting meshes visible from the light's camera, we collect ones in the light range
	// - for directional lights and PSSM this process will differ, may be extruded shadow boxes will be necessary
	// - collect ONLY shadow casters
	// - do not modify the main mesh list
	// - note: some caster meshes invisible from the main camera can have visible shadows. Anyway they are
	//         casting shadows on all visible receivers as their shadows are rendered to shadow buffer texture
	// - render casters to the shadow buffer texture
	// - end pass
	//
	// Posteffect path:
	// - begin pass
	// - Renderer: set posteffect shader
	// - Renderer: set source textures and other params
	// - Renderer: render fullscreen quad
	// - end pass
	//
	//
	// After this some UI, text and debug shapes can be rendered
	// Nebula treats all them as different batches or render plugins, it is good idea maybe...
	// Then backbuffer is present

	ClearVisibleLists();

	OK;
}
//---------------------------------------------------------------------

void CScene::SPSCollectVisibleObjects(CSPSNode* pNode, const matrix44& ViewProj,
									  CArray<CRenderObject*>* OutObjects, CArray<CLight*>* OutLights,
									  EClipStatus Clip)
{
	if (!pNode || !pNode->GetTotalObjCount() || (!OutObjects && !OutLights)) return;

	if (Clip == Clipped)
	{
		CAABB NodeBox;
		pNode->GetBounds(NodeBox);
		NodeBox.vmin.y = SceneBBox.vmin.y;
		NodeBox.vmax.y = SceneBBox.vmax.y;
		Clip = NodeBox.GetClipStatus(ViewProj);
		if (Clip == Outside) return;
	}

	if (OutObjects && pNode->Data.Objects.GetCount())
	{
		CArray<CSPSRecord*>::CIterator ItObj = pNode->Data.Objects.Begin();
		if (Clip == Inside)
		{
			CRenderObject** ppObj = OutObjects->Reserve(pNode->Data.Objects.GetCount());
			for (; ItObj != pNode->Data.Objects.End(); ++ItObj, ++ppObj)
				*ppObj = (CRenderObject*)&(*ItObj)->Attr;
		}
		else // Clipped
		{
			//???test against global box or transform to model space and test against local box?
			for (; ItObj != pNode->Data.Objects.End(); ++ItObj)
				if ((*ItObj)->GlobalBox.GetClipStatus(ViewProj) != Outside)
					OutObjects->Add((CRenderObject*)&(*ItObj)->Attr);
		}
	}

	if (OutLights && pNode->Data.Lights.GetCount())
	{
		CArray<CSPSRecord*>::CIterator ItLight = pNode->Data.Lights.Begin();
		if (Clip == Inside)
		{
			CLight** ppLight = OutLights->Reserve(pNode->Data.Lights.GetCount());
			for (; ItLight != pNode->Data.Lights.End(); ++ItLight, ++ppLight)
				*ppLight = (CLight*)&(*ItLight)->Attr;
		}
		else // Clipped
		{
			//???test against global box or transform to model space and test against local box?
			for (; ItLight != pNode->Data.Lights.End(); ++ItLight)
				if ((*ItLight)->GlobalBox.GetClipStatus(ViewProj) != Outside)
					OutLights->Add((CLight*)&(*ItLight)->Attr);
		}
	}

	if (pNode->HasChildren())
		for (DWORD i = 0; i < 4; i++)
			SPSCollectVisibleObjects(pNode->GetChild(i), ViewProj, OutObjects, OutLights, Clip);
}
//---------------------------------------------------------------------

bool CScene::OnRenderDeviceReset(const Events::CEventBase& Event)
{
	if (!AutoAdjustCameraAspect) FAIL;
	MainCamera->SetWidth((float)RenderSrv->GetBackBufferWidth());
	MainCamera->SetHeight((float)RenderSrv->GetBackBufferHeight());
	OK;
}
//---------------------------------------------------------------------

}
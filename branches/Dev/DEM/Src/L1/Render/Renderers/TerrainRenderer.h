#pragma once
#ifndef __DEM_L1_RENDER_TERRAIN_RENDERER_H__
#define __DEM_L1_RENDER_TERRAIN_RENDERER_H__

#include <Render/Renderer.h>
#include <Render/Materials/ShaderVar.h>
#include <Scene/Terrain.h>

// This terrain renderer implements CDLOD technique. It is paired with Scene::CTerrain
// object, that feeds the renderer with a necessary data.

//!!!Terrain lighting!

namespace Render
{

class CTerrainRenderer: public IRenderer
{
	DeclareRTTI;
	DeclareFactory(CTerrainRenderer);

protected:

	enum ENodeStatus
	{
		Node_Invisible,
		Node_NotInLOD,
		Node_Processed
	};

	struct CMorphInfo
	{
		float Start;
		float End;
		float Const1;	// end / (end - start)
		float Const2;	// 1 / (end - start)
	};

	struct CPatchInstance
	{
		vector4 ScaleOffset;
		vector4 GridToHM;
		float	MorphConsts[2];
	};

	PShader							Shader;
	CShaderVarMap					ShaderVars;
	DWORD							FeatFlags;
	bool							EnableLighting;

	float							VisibilityRange;
	float							MorphStartRatio;
	nArray<CMorphInfo>				MorphConsts;

	CShader::HVar					hHeightMap;
	CShader::HVar					hTerrainY;
	CShader::HVar					hGridConsts;
	CShader::HVar					hHMTexInfo;

	nDictionary<DWORD, CMesh*>		PatchMeshes;
	PVertexLayout					PatchVertexLayout;
	PVertexLayout					FinalVertexLayout;
	PVertexBuffer					InstanceBuffer;
	DWORD							MaxInstanceCount;

	//PShader							SharedShader;
	//CShader::HVar					hLightType;
	//CShader::HVar					hLightPos;
	//CShader::HVar					hLightDir;
	//CShader::HVar					hLightColor;
	//CShader::HVar					hLightParams;

	nArray<Scene::CTerrain*>		TerrainObjects;
	const nArray<Scene::CLight*>*	pLights;

	bool			CreatePatchMesh(DWORD Size);
	CMesh*			GetPatchMesh(DWORD Size);

	ENodeStatus		ProcessNode(Scene::CTerrain& Terrain, DWORD X, DWORD Z, DWORD LOD, float LODRange, DWORD& PatchCount, DWORD& QPatchCount, EClipStatus Clip = Clipped);

public:

	CTerrainRenderer();

	virtual bool	Init(const Data::CParams& Desc);
	virtual void	AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects);
	virtual void	AddLights(const nArray<Scene::CLight*>& Lights);
	virtual void	Render();

	void			SetVisibilityRange(float Range) { n_assert(Range > 0.f); VisibilityRange = Range; }
};

RegisterFactory(CTerrainRenderer);

typedef Ptr<CTerrainRenderer> PTerrainRenderer;

inline CTerrainRenderer::CTerrainRenderer():
	pLights(NULL),
	FeatFlags(0),
	EnableLighting(false),
	VisibilityRange(1000.f),
	MorphStartRatio(0.7f)
{
}
//---------------------------------------------------------------------

}

#endif

//???to the CTerrain?
/*
	nString PatchName;
	PatchName.Format("Patch%dx%d", PatchSize, PatchSize);
	Patch = RenderSrv->MeshMgr.GetTypedResource(CStrID(PatchName.Get()));
	if (!Patch->IsLoaded())
	{
		//!!!setup and set to loaded state!
		//Patch->Setup(VB, IB, MeshGroup array)
	}

	PatchName.Format("Patch%dx%d", PatchSize << 1, PatchSize << 1);
	QuarterPatch = RenderSrv->MeshMgr.GetTypedResource(CStrID(PatchName.Get()));
	if (!QuarterPatch->IsLoaded())
	{
		//!!!setup and set to loaded state!
		//QuarterPatch->Setup(VB, IB, MeshGroup array)
	}
*/

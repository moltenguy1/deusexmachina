#pragma once
#ifndef __DEM_L1_SCENE_MODEL_H__
#define __DEM_L1_SCENE_MODEL_H__

#include <Render/RenderObject.h>
#include <Render/Materials/Material.h>
#include <Render/Geometry/Mesh.h>
#include <Data/FixedArray.h>

// Mesh is a scene node attribute representing a visible shape.
// Mesh attribute references VB & IB resources, stores vertex and index range,
// material, its parameters and texture refs.

//!!!it is good to have attr for each mesh group, so separate visibility test
// is performed and shader sorting simplifies

class CAABB;

namespace Render
{
struct CSPSRecord;

class CModel: public CRenderObject
{
	__DeclareClass(CModel);

protected:

	virtual void	OnDetachFromNode();
	virtual bool	ValidateResources();

public:

	PMesh				Mesh;
	DWORD				MeshGroupIndex;
	PMaterial			Material;
	DWORD				FeatureFlags;	// Model shader flags like Skinned, must be ORed with material flags before use
	CShaderVarMap		ShaderVars;		// Animable per-object vars, also can store geom. vars like CullMode
	CStrID				BatchType;	//???use in CRenderObject and don't check RTTI at all?
	CFixedArray<int>	BoneIndices;	// For skinning splits due to shader constants limit only

	// ERenderFlag: ShadowCaster, ShadowReceiver, DoOcclusionCulling
	//can use Flags field of CNodeAttribute

	CSPSRecord*			pSPSRecord;

	CModel(): pSPSRecord(NULL), MeshGroupIndex(0), FeatureFlags(0) {}

	virtual bool	LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);

	virtual void	UpdateInSPS(CSPS& SPS, CArray<CRenderObject*>* pVisibleObjects);
	const CAABB&	GetLocalAABB() const { return Mesh->GetGroup(MeshGroupIndex).AABB; }
	void			GetGlobalAABB(CAABB& OutBox) const;
};

typedef Ptr<CModel> PModel;

}

#endif

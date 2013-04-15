#include "Terrain.h"

#include <Scene/Scene.h>
#include <Render/RenderServer.h>
#include <Data/Streams/FileStream.h>
#include <Data/DataServer.h>
#include <Data/BinaryReader.h>
#include <d3d9.h> //!!!for a texture format only!

namespace Render
{
	bool LoadTextureUsingD3DX(const nString& FileName, PTexture OutTexture);
}

namespace Scene
{
ImplementRTTI(Scene::CTerrain, Scene::CSceneNodeAttr);
ImplementFactory(Scene::CTerrain);

bool CTerrain::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'DLDC': // CDLD
		{
			HeightMap = RenderSrv->TextureMgr.GetTypedResource(DataReader.Read<CStrID>());
			OK;
		}
		case 'XSST': // TSSX
		{
			InvSplatSizeX = 1.f / DataReader.Read<float>();
			OK;
		}
		case 'ZSST': // TSSZ
		{
			InvSplatSizeZ = 1.f / DataReader.Read<float>();
			OK;
		}
		case 'SRAV': // VARS
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
			{
				CStrID VarName;
				DataReader.Read(VarName);
				CShaderVar& Var = ShaderVars.Add(VarName);
				Var.SetName(VarName);
				DataReader.Read(Var.Value);
			}
			OK;
		}
		case 'SXET': // TEXS
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
			{
				CStrID VarName;
				DataReader.Read(VarName);
				CShaderVar& Var = ShaderVars.Add(VarName);
				Var.SetName(VarName);
				Var.Value = RenderSrv->TextureMgr.GetTypedResource(DataReader.Read<CStrID>());
			}
			OK;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

bool CTerrain::OnAdd()
{
	Data::CFileStream CDLODFile;
	if (!CDLODFile.Open(HeightMap->GetUID().CStr(), Data::SAM_READ, Data::SAP_SEQUENTIAL)) FAIL;
	Data::CBinaryReader Reader(CDLODFile);

	n_assert(Reader.Read<int>() == 'CDLD');	// Magic
	n_assert(Reader.Read<int>() == 1);		// Version

	Reader.Read(HFWidth);
	Reader.Read(HFHeight);
	Reader.Read(PatchSize);
	Reader.Read(LODCount);
	DWORD MinMaxDataSize = Reader.Read<DWORD>();
	Reader.Read(VerticalScale);
	Reader.Read(Box.vmin.x);
	Reader.Read(Box.vmin.y);
	Reader.Read(Box.vmin.z);
	Reader.Read(Box.vmax.x);
	Reader.Read(Box.vmax.y);
	Reader.Read(Box.vmax.z);

	if (!HeightMap->IsLoaded())
	{
		//!!!write R32F variant!
		n_assert(RenderSrv->CheckCaps(Render::Caps_VSTex_L16));

		if (!HeightMap->Create(Render::CTexture::Texture2D, D3DFMT_L16, HFWidth, HFHeight, 0, 1, Render::Usage_Immutable, Render::CPU_NoAccess))
			FAIL;

		Render::CTexture::CMapInfo MapInfo;
		if (!HeightMap->Map(0, Map_Setup, MapInfo)) FAIL;
		CDLODFile.Read(MapInfo.pData, HFWidth * HFHeight * sizeof(unsigned short));
		HeightMap->Unmap(0);
		HeightMap->Setup();
	}
	else CDLODFile.Seek(HFWidth * HFHeight * sizeof(unsigned short), Data::SSO_CURRENT);

	pMinMaxData = (short*)n_malloc(MinMaxDataSize);
	CDLODFile.Read(pMinMaxData, MinMaxDataSize);

	//???store dimensions along with a pointer?
	DWORD PatchesW = (HFWidth - 1 + PatchSize - 1) / PatchSize;
	DWORD PatchesH = (HFHeight - 1 + PatchSize - 1) / PatchSize;
	DWORD Offset = 0;
	CMinMaxMap* pMMMap = MinMaxMaps.Reserve(LODCount);
	for (DWORD LOD = 0; LOD < LODCount; ++LOD, ++pMMMap)
	{
		pMMMap->PatchesW = PatchesW;
		pMMMap->PatchesH = PatchesH;
		pMMMap->pData = pMinMaxData + Offset;
		Offset += PatchesW * PatchesH * 2;
		PatchesW = (PatchesW + 1) / 2;
		PatchesH = (PatchesH + 1) / 2;
	}

	DWORD TopPatchSize = PatchSize << (LODCount - 1);
	TopPatchCountX = (HFWidth - 1 + TopPatchSize - 1) / TopPatchSize;
	TopPatchCountZ = (HFHeight - 1 + TopPatchSize - 1) / TopPatchSize;

	for (int i = 0; i < ShaderVars.Size(); ++i)
	{
		CShaderVar& Var = ShaderVars.ValueAtIndex(i);
		if (Var.Value.IsA<PTexture>())
		{
			PTexture Tex = Var.Value.GetValue<PTexture>();
			if (!Tex->IsLoaded()) LoadTextureUsingD3DX(Tex->GetUID().CStr(), Tex);
		}
	}

	OK;
}
//---------------------------------------------------------------------

void CTerrain::OnRemove()
{
	MinMaxMaps.Clear();
	SAFE_FREE(pMinMaxData);
	//HeightMap->Unload(); //???unload or leave in resource manager? leaving is good for save-load
}
//---------------------------------------------------------------------

void CTerrain::Update()
{
	/*if (pNode->IsWorldMatrixChanged())
	{
		//pNode->GetWorldMatrix();
	}*/

	//!!!can check global Box before adding!
	pNode->GetScene()->AddVisibleObject(*this);
}
//---------------------------------------------------------------------

void CTerrain::GetGlobalAABB(bbox3& Out) const
{
	const vector3& Translation = GetNode()->GetWorldPosition();
	Out.vmin = Box.vmin + Translation;
	Out.vmax = Box.vmax + Translation;
}
//---------------------------------------------------------------------

}
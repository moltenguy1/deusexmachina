#pragma once
#ifndef __DEM_L1_GFX_TEXTURE_H__
#define __DEM_L1_GFX_TEXTURE_H__

#include <Resources/Resource.h>
#include <Render/GPUResourceDefs.h>
#include <Render/D3D9Fwd.h>

// Texture resource, usable by renderer

//???!!!OnDeviceLost, OnDeviceReset! destroy & recreate

namespace Render
{

class CTexture: public Resources::CResource
{
	DeclareRTTI;

public:

	enum EType
	{
		InvalidType,
		Texture2D,
		Texture3D,
		TextureCube
	};

	enum ECubeFace
	{
		PosX = 0,
		NegX,
		PosY,
		NegY,
		PosZ,
		NegZ
	};

	struct CMapInfo
	{        
		void*	pData;
		int		RowPitch;
		int		DepthPitch;

		CMapInfo(): pData(NULL), RowPitch(0), DepthPitch(0) {}
	};

protected:

	EType					Type;
	DWORD					Width;
	DWORD					Height;
	DWORD					Depth;
	DWORD					MipCount;
	EPixelFormat			PixelFormat;

	DWORD					LockCount;

	IDirect3DBaseTexture9*	pD3D9Tex;

	union
	{
		IDirect3DTexture9*			pD3D9Tex2D;
		IDirect3DVolumeTexture9*	pD3D9Tex3D;
		IDirect3DCubeTexture9*		pD3D9TexCube;
	};

	void MapTypeToLockFlags(EMapType MapType, DWORD& LockFlags);

public:

	EUsage					Usage;
	ECPUAccess				AccessMode;
	//int						SkippedMips;

	CTexture(CStrID ID, Resources::IResourceManager* pHost);
	virtual ~CTexture() { if (IsLoaded()) Unload(); }

	//???Create(size, format etc), will setup itself inside to Loaded state?
	bool			Setup(IDirect3DBaseTexture9* pTextureCastToBase, EType TextureType);
	virtual void	Unload();

	bool			Map(int MipLevel, EMapType MapType, CMapInfo& OutMapInfo);
	void			Unmap(int MipLevel);
	bool			MapCubeFace(ECubeFace Face, int MipLevel, EMapType MapType, CMapInfo& OutMapInfo);
	void			UnmapCubeFace(ECubeFace Face, int MipLevel);

	EType						GetType() const { return Type; }
	DWORD						GetWidth() const { return Width; }
	DWORD						GetHeight() const { return Height; }
	DWORD						GetDepth() const { return Depth; }
	DWORD						GetMipLevelCount() const { return MipCount; }
	EPixelFormat				GetPixelFormat() const { return PixelFormat; }

	IDirect3DBaseTexture9*		GetD3D9BaseTexture() const { n_assert(!LockCount); return pD3D9Tex; }
	IDirect3DTexture9*			GetD3D9Texture() const { n_assert(!LockCount && Type == Texture2D); return pD3D9Tex2D; }
	IDirect3DCubeTexture9*		GetD3D9CubeTexture() const { n_assert(!LockCount && Type == TextureCube); return pD3D9TexCube; }
	IDirect3DVolumeTexture9*	GetD3D9VolumeTexture() const { n_assert(!LockCount && Type == Texture3D); return pD3D9Tex3D; }
};

typedef Ptr<CTexture> PTexture;

inline CTexture::CTexture(CStrID ID, Resources::IResourceManager* pHost):
	CResource(ID, pHost),
	Usage(UsageImmutable),
	AccessMode(AccessNone),
	Type(InvalidType),
	Width(0),
	Height(0),
	Depth(0),
	MipCount(0),
	//skippedMips(0),
	PixelFormat(InvalidPixelFormat),
	LockCount(0),
	pD3D9Tex(NULL),
	pD3D9Tex2D(NULL)
{
}
//---------------------------------------------------------------------

}

DECLARE_TYPE(Render::PTexture, 14)
#define TTexture DATA_TYPE(Render::PTexture)

#endif

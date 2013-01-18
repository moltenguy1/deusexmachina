#include "Texture.h"

#include <Render/RenderServer.h>

namespace Data
{
DEFINE_TYPE_EX(Render::PTexture, PTexture)
}

namespace Render
{
ImplementRTTI(Render::CTexture, Resources::CResource);

bool CTexture::Setup(IDirect3DBaseTexture9* pTextureCastToBase, EType TexType)
{
	n_assert(pTextureCastToBase);

	Type = TexType;

	//???really can't just cast ptr?
	// Need to query for base interface under Win32
	//???move query from switch?
	if (Type == Texture2D)
	{
		pD3D9Tex2D = (IDirect3DTexture9*)pTextureCastToBase;
		n_assert(SUCCEEDED(pD3D9Tex2D->QueryInterface(IID_IDirect3DBaseTexture9, (void**)&pD3D9Tex)));

		D3DSURFACE_DESC Desc;
		memset(&Desc, 0, sizeof(Desc));
		n_assert(SUCCEEDED(pD3D9Tex2D->GetLevelDesc(0, &Desc)));
		Width = Desc.Width;
		Height = Desc.Height;
		Depth = 1;
		PixelFormat = Desc.Format;
	}
	else if (Type == Texture3D)
	{
		pD3D9Tex3D = (IDirect3DVolumeTexture9*)pTextureCastToBase;
		n_assert(SUCCEEDED(pD3D9Tex3D->QueryInterface(IID_IDirect3DBaseTexture9, (void**)&pD3D9Tex)));

		D3DVOLUME_DESC Desc;
		memset(&Desc, 0, sizeof(Desc));
		n_assert(SUCCEEDED(pD3D9Tex3D->GetLevelDesc(0, &Desc)));
		Width = Desc.Width;
		Height = Desc.Height;
		Depth = Desc.Depth;
		PixelFormat = Desc.Format;
	}
	else if (Type == TextureCube)
	{
		pD3D9TexCube = (IDirect3DCubeTexture9*)pTextureCastToBase;
		n_assert(SUCCEEDED(pD3D9TexCube->QueryInterface(IID_IDirect3DBaseTexture9, (void**)&pD3D9Tex)));

		D3DSURFACE_DESC Desc;
		memset(&Desc, 0, sizeof(Desc));
		n_assert(SUCCEEDED(pD3D9TexCube->GetLevelDesc(0, &Desc)));
		Width = Desc.Width;
		Height = Desc.Height;
		PixelFormat = Desc.Format;
	}
	else
	{
		State = Resources::Rsrc_Failed;
		FAIL;
	}

	MipCount = pTextureCastToBase->GetLevelCount();

	State = Resources::Rsrc_Loaded;
	OK;
}
//---------------------------------------------------------------------

void CTexture::Unload()
{
	n_assert(!LockCount);
	switch (Type)
	{
		case Texture2D:		SAFE_RELEASE(pD3D9Tex2D); break;
		case Texture3D:		SAFE_RELEASE(pD3D9Tex3D); break;
		case TextureCube:	SAFE_RELEASE(pD3D9TexCube); break;
	}
	SAFE_RELEASE(pD3D9Tex);
	State = Resources::Rsrc_NotLoaded; //CResource::Unload();
}
//---------------------------------------------------------------------

bool CTexture::Create(EType _Type, D3DFORMAT _Format, DWORD _Width, DWORD _Height, DWORD _Depth, DWORD Mips,
					  EUsage _Usage, ECPUAccess _Access)
{
	if (!_Width || !_Height) FAIL;

	n_assert(!pD3D9Tex); //???or unload old?

	Usage = _Usage;
	Access = _Access;

	D3DPOOL D3DPool;
	DWORD D3DUsage;
	switch (Usage)
	{
		case UsageImmutable:
			n_assert(Access == AccessNone);
			D3DPool = D3DPOOL_MANAGED;
			D3DUsage = 0;
			break;
		case UsageDynamic:
			n_assert(Access == AccessWrite);
			D3DPool = D3DPOOL_DEFAULT;
			D3DUsage = D3DUSAGE_DYNAMIC;
			break;
		case UsageCPU:
			D3DPool = D3DPOOL_SYSTEMMEM;
			D3DUsage = D3DUSAGE_DYNAMIC;
			break;
		default: n_error("Invalid Texture usage!");
	}

	if (_Type == Texture2D)
	{
		HRESULT hr =
			RenderSrv->GetD3DDevice()->CreateTexture(_Width, _Height, Mips, D3DUsage, _Format, D3DPool, &pD3D9Tex2D, NULL);
		n_assert(SUCCEEDED(hr));
		return Setup(pD3D9Tex2D, _Type);
	}
	else if (_Type == Texture3D)
	{
		HRESULT hr =
			RenderSrv->GetD3DDevice()->CreateVolumeTexture(_Width, _Height, _Depth, Mips, D3DUsage, _Format, D3DPool, &pD3D9Tex3D, NULL);
		n_assert(SUCCEEDED(hr));
		return Setup(pD3D9Tex3D, _Type);
	}
	else if (_Type == TextureCube)
	{
		HRESULT hr =
			RenderSrv->GetD3DDevice()->CreateCubeTexture(_Width, Mips, D3DUsage, _Format, D3DPool, &pD3D9TexCube, NULL);
		n_assert(SUCCEEDED(hr));
		return Setup(pD3D9TexCube, _Type);
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CTexture::CreateRenderTarget(D3DFORMAT _Format, DWORD _Width, DWORD _Height)
{
	if (!_Width || !_Height) FAIL;

	n_assert(!pD3D9Tex); //???or unload old?

	Usage = UsageImmutable;
	Access = AccessNone;

	HRESULT hr = RenderSrv->GetD3DDevice()->CreateTexture(
		_Width,
		_Height,
		1,
		D3DUSAGE_RENDERTARGET,
		_Format,
		D3DPOOL_DEFAULT,
		&pD3D9Tex2D,
		NULL);
	n_assert(SUCCEEDED(hr));

	return Setup(pD3D9Tex2D, Texture2D);
}
//---------------------------------------------------------------------

inline void CTexture::MapTypeToLockFlags(EMapType MapType, DWORD& LockFlags)
{
	switch (MapType)
	{
		//!!!MapSetup!

		case MapRead:
			n_assert((UsageDynamic == Usage) && (AccessRead == Access));
			LockFlags |= D3DLOCK_READONLY;
			break;
		case MapWrite:
			n_assert((UsageDynamic == Usage) && (AccessWrite == Access));
			break;
		case MapReadWrite:
			n_assert((UsageDynamic == Usage) && (AccessReadWrite == Access));
			break;
		case MapWriteDiscard:
			n_assert((UsageDynamic == Usage) && (AccessWrite == Access));
			LockFlags |= D3DLOCK_DISCARD;
			break;
	}
}
//---------------------------------------------------------------------

bool CTexture::Map(int MipLevel, EMapType MapType, CMapInfo& OutMapInfo)
{
	n_assert((Type == Texture2D || Type == Texture3D) && MapType != MapWriteNoOverwrite);

	DWORD LockFlags = 0;
	MapTypeToLockFlags(MapType, LockFlags);

	if (Type == Texture2D)
	{
		D3DLOCKED_RECT LockedRect = { 0 };
		if (SUCCEEDED(GetD3D9Texture()->LockRect(MipLevel, &LockedRect, NULL, LockFlags)))
		{
			OutMapInfo.pData = LockedRect.pBits;
			OutMapInfo.RowPitch = LockedRect.Pitch;
			OutMapInfo.DepthPitch = 0;
			LockCount++;
			OK;
		}
	}
	else if (Type == Texture3D)
	{
		D3DLOCKED_BOX LockedBox = { 0 };
		if (SUCCEEDED(GetD3D9VolumeTexture()->LockBox(MipLevel, &LockedBox, NULL, LockFlags)))
		{
			OutMapInfo.pData = LockedBox.pBits;
			OutMapInfo.RowPitch = LockedBox.RowPitch;
			OutMapInfo.DepthPitch = LockedBox.SlicePitch;
			LockCount++;
			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

void CTexture::Unmap(int MipLevel)
{
	n_assert(LockCount > 0);
	if (Texture2D == Type) GetD3D9Texture()->UnlockRect(MipLevel);
	else if (Texture3D == Type) GetD3D9VolumeTexture()->UnlockBox(MipLevel);
	else n_error("CTexture::Unmap -> Cube texture, use UnmapCubeFace");
	LockCount--;
}
//---------------------------------------------------------------------

bool CTexture::MapCubeFace(ECubeFace Face, int MipLevel, EMapType MapType, CMapInfo& OutMapInfo)
{
	n_assert(Type == TextureCube && MapType != MapWriteNoOverwrite);

	DWORD LockFlags = D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK;
	MapTypeToLockFlags(MapType, LockFlags);

	D3DLOCKED_RECT LockedRect = { 0 };
	if (FAILED(GetD3D9CubeTexture()->LockRect((D3DCUBEMAP_FACES)Face, MipLevel, &LockedRect, NULL, LockFlags))) FAIL;

	OutMapInfo.pData = LockedRect.pBits;
	OutMapInfo.RowPitch = LockedRect.Pitch;
	OutMapInfo.DepthPitch = 0;
	LockCount++;
	OK;
}
//---------------------------------------------------------------------

void CTexture::UnmapCubeFace(ECubeFace Face, int MipLevel)
{
	n_assert(Type == TextureCube && LockCount > 0);
	GetD3D9CubeTexture()->UnlockRect((D3DCUBEMAP_FACES)Face, MipLevel);
	LockCount--;
}
//---------------------------------------------------------------------

void CTexture::GenerateMipLevels()
{
	// To have mipmap sublevels generated automatically at texture
	// creation time, specify D3DUSAGE_AUTOGENMIPMAP (c) D3D9 docs
	n_assert(pD3D9Tex);
	pD3D9Tex->GenerateMipSubLevels();
}
//---------------------------------------------------------------------

}
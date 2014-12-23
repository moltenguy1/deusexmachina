#include "D3D9GPUDriver.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#include <Render/D3D9/D3D9DisplayDriver.h>
#include <Events/EventServer.h>
#include <System/OSWindow.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CD3D9GPUDriver, 'D9GD', Render::CGPUDriver);

void CD3D9GPUDriver::CD3D9SwapChain::Release()
{
	Sub_OnClosing = NULL;
	Sub_OnSizeChanged = NULL;
	Sub_OnToggleFullscreen = NULL;

	if (pSwapChain)
	{
		pSwapChain->Release();
		pSwapChain = NULL;
	}
	else
	{
		//???move this branch to a DestroySwapChain() method?
		Sys::Error("CD3D9GPUDriver::CD3D9SwapChain::Release() > Implicit swap chain destroyed! IMPLEMENT ME!!!");
		// Implicit swap chain
		//!!!destroy device!
		//???or deny and require user to call DestroyDevice (Term)?
	}
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::Reset(D3DPRESENT_PARAMETERS& D3DPresentParams)
{
	if (!pD3DDevice) FAIL;

	Sub_OnPaint = NULL;

	for (int i = 0; i < SwapChains.GetCount() ; ++i)
		SwapChains[i].Release();

	//???call some Release() code instead? kill all except the device itself!
	EventSrv->FireEvent(CStrID("OnRenderDeviceLost"));

	//!!!ReleaseQueries();
	//SAFE_RELEASE(pCurrDSSurface);

	Sys::COSWindow* pFocusWnd = D3D9DrvFactory->GetFocusWindow();

	HRESULT hr = pD3DDevice->TestCooperativeLevel();
	while (hr != S_OK && hr != D3DERR_DEVICENOTRESET)
	{
		// NB: In single-threaded app, engine will stuck here until device can be reset
		Sys::Sleep(10);
		if (pFocusWnd) pFocusWnd->ProcessMessages();
		hr = pD3DDevice->TestCooperativeLevel();
	}

	hr = pD3DDevice->Reset(&D3DPresentParams);
	if (FAILED(hr))
	{
		//!!!DXERR!
		//Sys::Log("Failed to reset Direct3D9 device object: %s!\n", DXGetErrorString(hr));
		Sys::Log("Failed to reset Direct3D9 device object!\n");
		FAIL;
	}

	// Can setup W-buffer: D3DCaps.RasterCaps | D3DPRASTERCAPS_WBUFFER -> D3DZB_USEW

	pD3DDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pD3DDevice->SetRenderState(D3DRS_FILLMODE, /*Wireframe ? D3DFILL_WIREFRAME :*/ D3DFILL_SOLID);

	//!!!only to decide whether to clear stencil! clear if current DS surface includes stencil bits
	//???!!!RT must not have embedded DS surface, it is a separate class!
	//so there is no need in this field!
	//CurrDepthStencilFormat =
	//	D3DPresentParams.EnableAutoDepthStencil ?  D3DPresentParams.AutoDepthStencilFormat : D3DFMT_UNKNOWN;

	if (D3DPresentParams.Windowed == TRUE)
	{
		bool Failed = false;

		for (int i = 0; i < SwapChains.GetCount() ; ++i)
		{
			CD3D9SwapChain& SC = SwapChains[i];

			// Skip implicit swap chain, index is always 0
			if (i != 0)
			{
				D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
				if (!GetCurrD3DPresentParams(SC, D3DPresentParams) ||
					FAILED(pD3DDevice->CreateAdditionalSwapChain(&D3DPresentParams, &SC.pSwapChain)))
				{
					DestroySwapChain(i);
					Failed = false;
				}
			}

			SC.TargetWindow->Subscribe<CD3D9GPUDriver>(CStrID("OnToggleFullscreen"), this, &CD3D9GPUDriver::OnOSWindowToggleFullscreen, &SC.Sub_OnToggleFullscreen);
			SC.TargetWindow->Subscribe<CD3D9GPUDriver>(CStrID("OnClosing"), this, &CD3D9GPUDriver::OnOSWindowClosing, &SC.Sub_OnClosing);
			if (SC.Desc.Flags.Is(SwapChain_AutoAdjustSize))
				SC.TargetWindow->Subscribe<CD3D9GPUDriver>(CStrID("OnSizeChanged"), this, &CD3D9GPUDriver::OnOSWindowSizeChanged, &SC.Sub_OnSizeChanged);
		}

		if (Failed)
		{
			// Partially failed, can handle specifically
			FAIL;
		}
	}
	else SwapChains[0].TargetWindow->Subscribe<CD3D9GPUDriver>(CStrID("OnPaint"), this, &CD3D9GPUDriver::OnOSWindowPaint, &Sub_OnPaint);

	EventSrv->FireEvent(CStrID("OnRenderDeviceReset"));

	//pCurrDSSurface = DefaultRT->GetD3DDepthStencilSurface();

	IsInsideFrame = false;

	OK;
}
//---------------------------------------------------------------------

void CD3D9GPUDriver::Release()
{
	if (!pD3DDevice) return;

	Sub_OnPaint = NULL;

	//!!!if code won't be reused in Reset(), call DestroySwapChain()!
	for (int i = 0; i < SwapChains.GetCount() ; ++i)
		SwapChains[i].Release();

	//!!!UnbindD3D9Resources();
	//!!!can call the same event as on lost device!

	//for (int i = 1; i < MaxRenderTargetCount; i++)
	//	pD3DDevice->SetRenderTarget(i, NULL);
	pD3DDevice->SetDepthStencilSurface(NULL); //???need when auto depth stencil?

	//EventSrv->FireEvent(CStrID("OnRenderDeviceRelease"));

	//!!!ReleaseQueries();

	pD3DDevice->Release();
	pD3DDevice = NULL;

	IsInsideFrame = false;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::CheckCaps(ECaps Cap)
{
	n_assert(pD3DDevice);

	switch (Cap)
	{
		case Caps_VSTexFiltering_Linear:
			return (D3DCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR) && (D3DCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR);
		case Caps_VSTex_L16:
			return SUCCEEDED(D3D9DrvFactory->GetDirect3D9()->CheckDeviceFormat(	Adapter,
																				DEM_D3D_DEVICETYPE,
																				D3DFMT_UNKNOWN, //D3DPresentParams.BackBufferFormat,
																				D3DUSAGE_QUERY_VERTEXTEXTURE,
																				D3DRTYPE_TEXTURE,
																				D3DFMT_L16));
		default: FAIL;
	}
}
//---------------------------------------------------------------------

void CD3D9GPUDriver::FillD3DPresentParams(const CSwapChainDesc& Desc, const Sys::COSWindow* pWindow, D3DPRESENT_PARAMETERS& D3DPresentParams)
{
	DWORD BackBufferCount = Desc.BackBufferCount ? Desc.BackBufferCount : 1;

	switch (Desc.SwapMode)
	{
		case SwapMode_CopyPersist:	D3DPresentParams.SwapEffect = BackBufferCount > 1 ? D3DSWAPEFFECT_FLIP : D3DSWAPEFFECT_COPY; break;
		//case SwapMode_FlipPersist:	// D3DSWAPEFFECT_FLIPEX, but it is available only in D3D9Ex
		case SwapMode_CopyDiscard:
		default:					D3DPresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD; break; // Allows runtime to select the best
	}

	if (D3DPresentParams.SwapEffect == D3DSWAPEFFECT_DISCARD)
	{
		// It is recommended to use non-MSAA swap chain, render to MSAA RT and then resolve it to the back buffer
		//???support MSAA or enforce separate MSAA RT?
		D3DPresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
		D3DPresentParams.MultiSampleQuality = 0;
	}
	else
	{
		// MSAA not supported for swap effects other than D3DSWAPEFFECT_DISCARD
		D3DPresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
		D3DPresentParams.MultiSampleQuality = 0;
	}

	D3DPresentParams.hDeviceWindow = pWindow->GetHWND();
	D3DPresentParams.BackBufferCount = BackBufferCount; //!!!N3 always sets 1 in windowed mode! why?
	D3DPresentParams.EnableAutoDepthStencil = TRUE;
	D3DPresentParams.AutoDepthStencilFormat = D3DFMT_D24S8; // D3DFMT_D32 if no need in stencil

	// D3DPRESENT_INTERVAL_ONE - as _DEFAULT, but improves VSync quality at a little cost of processing time (uses another timer)
	D3DPresentParams.PresentationInterval = Desc.Flags.Is(SwapChain_VSync) ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;
	D3DPresentParams.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	D3DPresentParams.Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;

#if DEM_D3D_DEBUG
	D3DPresentParams.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
#endif
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::GetCurrD3DPresentParams(const CD3D9SwapChain& SC, D3DPRESENT_PARAMETERS& D3DPresentParams)
{
	FillD3DPresentParams(SC.Desc, SC.TargetWindow, D3DPresentParams);

	if (SC.IsFullscreen())
	{
		CDisplayMode Mode;
		if (!SC.pTargetDisplay->GetCurrentDisplayMode(Mode)) FAIL;
		D3DPresentParams.Windowed = FALSE;
		D3DPresentParams.BackBufferWidth = Mode.Width;
		D3DPresentParams.BackBufferHeight = Mode.Height;
		D3DPresentParams.BackBufferFormat = CD3D9DriverFactory::PixelFormatToD3DFormat(Mode.PixelFormat);
		//D3DPresentParams.FullScreen_RefreshRateInHz = Mode.RefreshRate.GetIntRounded();
	}
	else
	{
		D3DPresentParams.Windowed = TRUE;
		D3DPresentParams.BackBufferWidth = SC.Desc.BackBufferWidth;
		D3DPresentParams.BackBufferHeight = SC.Desc.BackBufferHeight;
		D3DPresentParams.BackBufferFormat = D3DFMT_UNKNOWN; // Uses current desktop mode
	}

	OK;
}
//---------------------------------------------------------------------

// If device exists, creates additional swap chain. If device not exist, creates a device with an implicit swap chain.
DWORD CD3D9GPUDriver::CreateSwapChain(const CSwapChainDesc& Desc, Sys::COSWindow* pWindow)
{
	IDirect3D9* pD3D9 = D3D9DrvFactory->GetDirect3D9();

#if DEM_D3D_USENVPERFHUD
	Sys::Error("IMPLEMENT ME!!! NVPerfHUD.");
//for (UINT CurrAdapter = 0; CurrAdapter < pD3D9->GetAdapterCount(); ++CurrAdapter) 
//{ 
//	D3DADAPTER_IDENTIFIER9 Identifier;
//	HRESULT Res = pD3D9->GetAdapterIdentifier(CurrAdapter, 0, &Identifier);
//	if (strstr(Identifier.Description, "PerfHUD") != 0)
//	{
//		Adapter = CurrAdapter;
//		DeviceType = D3DDEVTYPE_REF;
//		break;
//	}
//} 
#endif

	if (D3D9DrvFactory->AdapterExists(Adapter))
	{
		Sys::Error("Inexistent adapter specified on D3D9 swap chain creation!\n");
		return ERR_CREATION_ERROR;
	}

	memset(&D3DCaps, 0, sizeof(D3DCaps));
	n_assert(SUCCEEDED(pD3D9->GetDeviceCaps(Adapter, DEM_D3D_DEVICETYPE, &D3DCaps)));

	Sys::COSWindow* pWnd = pWindow ? pWindow : D3D9DrvFactory->GetFocusWindow();
	n_assert(pWnd);

	//???check all the swap chains not to use this window?

	// Zero means matching window or display size.
	// But if at least one of these values specified, we should adjst window size.
	// A child window is an exception, we don't want rederer to resize it,
	// so we force a backbuffer size to a child window size.
	UINT BBWidth = Desc.BackBufferWidth, BBHeight = Desc.BackBufferHeight;
	if (BBWidth > 0 || BBHeight > 0)
	{
		if (pWnd->IsChild())
		{
			BBWidth = pWnd->GetWidth();
			BBHeight = pWnd->GetHeight();
		}
		else
		{
			Data::CRect WindowRect = pWnd->GetRect();
			if (BBWidth > 0) WindowRect.W = BBWidth;
			else BBWidth = WindowRect.W;
			if (BBHeight > 0) WindowRect.H = BBHeight;
			else BBHeight = WindowRect.H;
			pWnd->SetRect(WindowRect);
		}
	}

	//???why it is better to create windowed swap chain and then turn it fullscreen? except that it uses less arguments on creation.
	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	FillD3DPresentParams(Desc, pWnd, D3DPresentParams);

	D3DPresentParams.Windowed = TRUE;
	D3DPresentParams.BackBufferWidth = BBWidth;
	D3DPresentParams.BackBufferHeight = BBHeight;
	D3DPresentParams.BackBufferFormat = D3DFMT_UNKNOWN; // Uses current desktop mode

	// Make sure the device supports a depth buffer specified
	//???does support D3DFMT_UNKNOWN? if not, need to explicitly determine adapter format.
	HRESULT hr = pD3D9->CheckDeviceFormat(	Adapter,
											DEM_D3D_DEVICETYPE,
											D3DPresentParams.BackBufferFormat,
											D3DUSAGE_DEPTHSTENCIL,
											D3DRTYPE_SURFACE,
											D3DPresentParams.AutoDepthStencilFormat);
	if (FAILED(hr))
	{
		Sys::Error("Rendering device doesn't support D24S8 depth buffer!\n");
		return ERR_CREATION_ERROR;
	}

	// Check that the depth buffer format is compatible with the backbuffer format
	//???does support D3DFMT_UNKNOWN? if not, need to explicitly determine adapter (and also backbuffer) format.
	hr = pD3D9->CheckDepthStencilMatch(	Adapter,
										DEM_D3D_DEVICETYPE,
										D3DPresentParams.BackBufferFormat,
										D3DPresentParams.BackBufferFormat,
										D3DPresentParams.AutoDepthStencilFormat);
	if (FAILED(hr))
	{
		Sys::Error("Backbuffer format is not compatible with D24S8 depth buffer!\n");
		return ERR_CREATION_ERROR;
	}

	CArray<CD3D9SwapChain>::CIterator ItSC = NULL;

	if (pD3DDevice)
	{
		// As device exists, implicit swap chain exists too, so create additional one.
		// NB: additional swap chains work only in windowed mode

		//???must specify unique window or can render into a focus window?

		IDirect3DSwapChain9* pD3DSwapChain = NULL;
		hr = pD3DDevice->CreateAdditionalSwapChain(&D3DPresentParams, &pD3DSwapChain);

		if (FAILED(hr))
		{
			//!!!DX9! Sys::Error("Failed to create additional Direct3D9 swap chain: %s!\n", DXGetErrorString(hr));
			return ERR_CREATION_ERROR;
		}

		for (ItSC = SwapChains.Begin(); ItSC != SwapChains.End(); ++ItSC)
			if (!ItSC->IsValid()) break;

		if (ItSC == SwapChains.End())
		{
			//if (SwapChains.GetCount() >= MaxSwapChainCount) return ERR_MAX_SWAP_CHAIN_COUNT_EXCEEDED;
			ItSC = SwapChains.Reserve(1);
		}
	
		ItSC->pSwapChain = pD3DSwapChain;
	}
	else
	{
		// There is no swap chain nor device, create D3D9 device with an implicit swap chain

		n_assert(!SwapChainExists(0));

#if DEM_D3D_DEBUG
		DWORD BhvFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_SOFTWARE_VERTEXPROCESSING;
#else
		DWORD BhvFlags = (D3DCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) ?
			D3DCREATE_HARDWARE_VERTEXPROCESSING :
			D3DCREATE_SOFTWARE_VERTEXPROCESSING;
#endif

		// NB: May fail if can't create requested number of backbuffers
		hr = pD3D9->CreateDevice(Adapter,
								DEM_D3D_DEVICETYPE,
								D3D9DrvFactory->GetFocusWindow()->GetHWND(),
								BhvFlags,
								&D3DPresentParams,
								&pD3DDevice);

		if (FAILED(hr))
		{
			//!!!DX9! Sys::Error("Failed to create Direct3D9 device object: %s!\n", DXGetErrorString(hr));
			Sys::Error("Failed to create Direct3D9 device object!\n");
			return ERR_CREATION_ERROR;
		}

		// Can setup W-buffer: D3DCaps.RasterCaps | D3DPRASTERCAPS_WBUFFER -> D3DZB_USEW

		pD3DDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);
		pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
		pD3DDevice->SetRenderState(D3DRS_FILLMODE, /*Wireframe ? D3DFILL_WIREFRAME :*/ D3DFILL_SOLID);

		ItSC = SwapChains.IteratorAt(0); 
		ItSC->pSwapChain = NULL;
	}

	//!!!only to decide whether to clear stencil! clear if current DS surface includes stencil bits
	//???!!!RT must not have embedded DS surface, it is a separate class!
	//so there is no need in this field!
	//CurrDepthStencilFormat =
	//	D3DPresentParams.EnableAutoDepthStencil ?  D3DPresentParams.AutoDepthStencilFormat : D3DFMT_UNKNOWN;

	ItSC->Desc = Desc;
	ItSC->TargetWindow = pWnd;
	ItSC->LastWindowRect = pWnd->GetRect();

	pWnd->Subscribe<CD3D9GPUDriver>(CStrID("OnToggleFullscreen"), this, &CD3D9GPUDriver::OnOSWindowToggleFullscreen, &ItSC->Sub_OnToggleFullscreen);
	pWnd->Subscribe<CD3D9GPUDriver>(CStrID("OnClosing"), this, &CD3D9GPUDriver::OnOSWindowClosing, &ItSC->Sub_OnClosing);
	if (Desc.Flags.Is(SwapChain_AutoAdjustSize))
		pWnd->Subscribe<CD3D9GPUDriver>(CStrID("OnSizeChanged"), this, &CD3D9GPUDriver::OnOSWindowSizeChanged, &ItSC->Sub_OnSizeChanged);

	return SwapChains.IndexOf(ItSC);
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::DestroySwapChain(DWORD SwapChainID)
{
	if (!SwapChainExists(SwapChainID)) FAIL;
	
	CD3D9SwapChain& SC = SwapChains[SwapChainID];
	SC.Release();
	SC.pTargetDisplay = NULL;
	SC.TargetWindow = NULL;

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::SwapChainExists(DWORD SwapChainID) const
{
	return SwapChainID < (DWORD)SwapChains.GetCount() && SwapChains[SwapChainID].IsValid();
}
//---------------------------------------------------------------------

// Does not resize an OS window, since often is called in response to an OS window resize
//???bool ResizeOSWindow?
bool CD3D9GPUDriver::ResizeSwapChain(DWORD SwapChainID, unsigned int Width, unsigned int Height)
{
	if (!SwapChainExists(SwapChainID)) FAIL;

	CD3D9SwapChain& SC = SwapChains[SwapChainID];

	if ((!Width || SC.Desc.BackBufferWidth == Width) && (!Height || SC.Desc.BackBufferHeight == Height)) OK;

	//???for child window, assert that size passed is a window size?

	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	if (!GetCurrD3DPresentParams(SC, D3DPresentParams)) FAIL;

	if (SC.IsFullscreen())
	{
		CDisplayMode Mode(
			Width ? Width : D3DPresentParams.BackBufferWidth,
			Height ? Height : D3DPresentParams.BackBufferHeight,
			CD3D9DriverFactory::D3DFormatToPixelFormat(D3DPresentParams.BackBufferFormat));
		if (!SC.pTargetDisplay->SupportsDisplayMode(Mode))
		{
			//!!!if (!SC.pTargetDisplay->GetClosestDisplayMode(inout Mode)) FAIL;!
			FAIL;
		}
		D3DPresentParams.BackBufferWidth = Mode.Width;
		D3DPresentParams.BackBufferHeight = Mode.Height;
	}
	else
	{
		if (Width) D3DPresentParams.BackBufferWidth = Width;
		if (Height) D3DPresentParams.BackBufferHeight = Height;
	}

	if (SC.pSwapChain)
	{
		SC.pSwapChain->Release();
		if (FAILED(pD3DDevice->CreateAdditionalSwapChain(&D3DPresentParams, &SC.pSwapChain))) FAIL;
	}
	else if (!Reset(D3DPresentParams)) FAIL;

	SC.Desc.BackBufferWidth = D3DPresentParams.BackBufferWidth;
	SC.Desc.BackBufferHeight = D3DPresentParams.BackBufferHeight;

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::SwitchToFullscreen(DWORD SwapChainID, const CDisplayDriver* pDisplay, const CDisplayMode* pMode)
{
	if (!SwapChainExists(SwapChainID)) FAIL;
	if (pDisplay && Adapter != pDisplay->GetAdapterID()) FAIL;

	CD3D9SwapChain& SC = SwapChains[SwapChainID];

	// Only one swap chain per adapter can be fullscreen in D3D9
	// Moreover, it is always an implicit swap chain, so fail on any additional one
	if (SC.pSwapChain) FAIL;

	//???if (SC.TargetWindow->IsChild()) FAIL;

	if (!pDisplay)
	{
		Sys::Error("CD3D9GPUDriver::SwitchToFullscreen > IMPLEMENT ME for pDisplay == NULL!!!");
		// system will select from window
		// or for d3d get adapter display format directly
	}

	CDisplayMode CurrentMode;
	if (!pMode)
	{
		if (!pDisplay->GetCurrentDisplayMode(CurrentMode)) FAIL;
		pMode = &CurrentMode;
	}

	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	FillD3DPresentParams(SC.Desc, SC.TargetWindow, D3DPresentParams);

	D3DPresentParams.Windowed = FALSE;
	D3DPresentParams.BackBufferWidth = pMode->Width;
	D3DPresentParams.BackBufferHeight = pMode->Height;
	D3DPresentParams.BackBufferFormat = CD3D9DriverFactory::PixelFormatToD3DFormat(pMode->PixelFormat);

	CDisplayDriver::CMonitorInfo MonInfo;
	if (!pDisplay->GetDisplayMonitorInfo(MonInfo)) FAIL;
	SC.LastWindowRect = SC.TargetWindow->GetRect();
	SC.TargetWindow->SetRect(Data::CRect(MonInfo.Left, MonInfo.Top, pMode->Width, pMode->Height), true);

	if (!Reset(D3DPresentParams)) FAIL;

	SC.Desc.BackBufferWidth = D3DPresentParams.BackBufferWidth;
	SC.Desc.BackBufferHeight = D3DPresentParams.BackBufferHeight;

	SC.pTargetDisplay = pDisplay;
	SC.TargetWindow->Subscribe<CD3D9GPUDriver>(CStrID("OnPaint"), this, &CD3D9GPUDriver::OnOSWindowPaint, &Sub_OnPaint);

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::SwitchToWindowed(DWORD SwapChainID, const Data::CRect* pWindowRect)
{
	if (!SwapChainExists(SwapChainID)) FAIL;

	CD3D9SwapChain& SC = SwapChains[SwapChainID];

	// Only one swap chain per adapter can be fullscreen in D3D9.
	// Moreover, it is always an implicit swap chain, so skip transition for
	// any additional swap chain, as it always is in a windowed mode.
	// Use ResizeSwapChain() to change the swap chain size.
	if (SC.pSwapChain) OK;

	if (pWindowRect)
	{
		SC.LastWindowRect.X = pWindowRect->X;
		SC.LastWindowRect.Y = pWindowRect->Y;
		if (pWindowRect->W > 0) SC.LastWindowRect.W = pWindowRect->W;
		if (pWindowRect->H > 0) SC.LastWindowRect.H = pWindowRect->H;
	}

	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	FillD3DPresentParams(SC.Desc, SC.TargetWindow, D3DPresentParams);

	D3DPresentParams.Windowed = TRUE;
	D3DPresentParams.BackBufferWidth = SC.LastWindowRect.W;
	D3DPresentParams.BackBufferHeight = SC.LastWindowRect.H;
	D3DPresentParams.BackBufferFormat = D3DFMT_UNKNOWN; // Uses current desktop mode

	if (!Reset(D3DPresentParams)) FAIL;

	SC.TargetWindow->SetRect(SC.LastWindowRect);

	SC.Desc.BackBufferWidth = D3DPresentParams.BackBufferWidth;
	SC.Desc.BackBufferHeight = D3DPresentParams.BackBufferHeight;

	SC.pTargetDisplay = NULL;

	Sub_OnPaint = NULL;

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::IsFullscreen(DWORD SwapChainID) const
{
	return SwapChainExists(SwapChainID) && SwapChains[SwapChainID].IsFullscreen();
}
//---------------------------------------------------------------------

// NB: Present() should be called as late as possible after EndFrame()
// to improve parallelism between the GPU and the CPU
bool CD3D9GPUDriver::Present(DWORD SwapChainID)
{
	if (IsInsideFrame || !SwapChainExists(SwapChainID)) FAIL;

	CD3D9SwapChain& SC = SwapChains[SwapChainID];

	// For swap chain: Present will fail if called between BeginScene and EndScene pairs unless the
	// render target is not the current render target. //???so don't fail if IsInsideFrame?
	HRESULT hr = SC.pSwapChain ? SC.pSwapChain->Present(NULL, NULL, NULL, NULL, 0) : pD3DDevice->Present(NULL, NULL, NULL, NULL);
	if (FAILED(hr))
	{
		if (hr == D3DERR_DEVICELOST)
		{
			CD3D9SwapChain& ImplicitSC = SwapChains[0];

			D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
			if (!GetCurrD3DPresentParams(ImplicitSC, D3DPresentParams)) FAIL;
			if (!Reset(D3DPresentParams)) FAIL;
		}
		else if (hr == D3DERR_INVALIDCALL) FAIL;
		else // D3DERR_DRIVERINTERNALERROR, D3DERR_OUTOFVIDEOMEMORY, E_OUTOFMEMORY
		{
			// Destroy and recreate device and all swap chains
			Sys::Error("CD3D9GPUDriver::Present() > IMPLEMENT ME FAILED(hr)!!!");
			FAIL;
		}
	}
	else ++SC.FrameID;

	//// Sync CPU thread with GPU
	//// wait till gpu has finsihed rendering the previous frame
	//gpuSyncQuery[frameId % numSyncQueries]->Issue(D3DISSUE_END);                              
	//++FrameID; //???why here?
	//while (S_FALSE == gpuSyncQuery[frameId % numSyncQueries]->GetData(NULL, 0, D3DGETDATA_FLUSH)) ;

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::OnOSWindowToggleFullscreen(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Sys::COSWindow* pWnd = (Sys::COSWindow*)P->Get<PVOID>(CStrID("Window"));
	for (int i = 0; i < SwapChains.GetCount(); ++i)
		if (SwapChains[i].TargetWindow.GetUnsafe() == pWnd)
		{
			if (SwapChains[i].IsFullscreen()) return SwitchToWindowed(i);
			else return SwitchToFullscreen(i);
		}
	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::OnOSWindowSizeChanged(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Sys::COSWindow* pWnd = (Sys::COSWindow*)P->Get<PVOID>(CStrID("Window"));
	for (int i = 0; i < SwapChains.GetCount(); ++i)
		if (SwapChains[i].TargetWindow.GetUnsafe() == pWnd)
		{
			// May not fire in fullscreen mode by design, this assert checks it
			// If assertion failed, we should rewrite this handler and maybe some other code
			n_assert_dbg(!SwapChains[i].IsFullscreen());

			ResizeSwapChain(i, pWnd->GetWidth(), pWnd->GetHeight());
			OK; // Only one swap chain is allowed for each window
		}
	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::OnOSWindowPaint(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Sys::COSWindow* pWnd = (Sys::COSWindow*)P->Get<PVOID>(CStrID("Window"));
	for (int i = 0; i < SwapChains.GetCount(); ++i)
	{
		CD3D9SwapChain& SC = SwapChains[i];
		if (SC.TargetWindow.GetUnsafe() == pWnd)
		{
			n_assert_dbg(SC.IsFullscreen());
			if (SC.pSwapChain) SC.pSwapChain->Present(NULL, NULL, NULL, NULL, 0);
			else pD3DDevice->Present(NULL, NULL, NULL, NULL);
			OK; // Only one swap chain is allowed for each window
		}
	}
	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::OnOSWindowClosing(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Sys::COSWindow* pWnd = (Sys::COSWindow*)P->Get<PVOID>(CStrID("Window"));
	for (int i = 0; i < SwapChains.GetCount(); ++i)
		if (SwapChains[i].TargetWindow.GetUnsafe() == pWnd)
		{
			DestroySwapChain(i);
			OK; // Only one swap chain is allowed for each window
		}
	OK;
}
//---------------------------------------------------------------------

}

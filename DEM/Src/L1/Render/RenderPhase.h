#pragma once
#ifndef __DEM_L1_RENDER_PHASE_H__
#define __DEM_L1_RENDER_PHASE_H__

#include <Core/Object.h>
#include <Data/FixedArray.h>

// Rendering phase implements a process of rendering into RT, MRT or DSV, in various forms,
// including color rendering, Z-prepass, occlusion culling, shadow map, fullscreen quad for
// postprocessing effects and others.

namespace Data
{
	class CParams;
}

namespace Render
{
class CRenderObject;
class CLight;
typedef Ptr<class CRenderTarget> PRenderTarget;
typedef Ptr<class CDepthStencilBuffer> PDepthStencilBuffer;

class CRenderPhase: public Core::CObject
{
public:

	CFixedArray<PRenderTarget>	RenderTargets;
	CFixedArray<DWORD>			RTClearColors;		// 32-bit ARGB each
	PDepthStencilBuffer			DepthStencil;
	float						DepthClearValue;	// 0.f - 1.f
	uchar						StencilClearValue;
	Data::CFlags				ClearFlags;

	CRenderPhase(): ClearFlags(0), DepthClearValue(1.f), StencilClearValue(0) {}

	virtual ~CRenderPhase() {}

	virtual bool Init(const Data::CParams& Desc);
	virtual void Render() = 0;
};

typedef Ptr<CRenderPhase> PRenderPhase;

}

#endif

#pragma once
#ifndef __DEM_L1_RENDER_H__
#define __DEM_L1_RENDER_H__

#include <Render/D3D9Fwd.h>
#include <kernel/ntypes.h>

// Render system definitions and forward declarations

namespace Render
{

enum EClearFlag
{
	Clear_Color		= 0x01,
	Clear_Depth		= 0x02,
	Clear_Stencil	= 0x04
};

enum EMSAAQuality
{
	MSAA_None	= 0,
	MSAA_2x		= 2,
	MSAA_4x		= 4,
	MSAA_8x		= 8
};

struct CMonitorInfo
{
	ushort	Left;
	ushort	Top;
	ushort	Width;
	ushort	Height;
	bool	IsPrimary;
};

enum EPrimitiveTopology
{
	PointList,
	LineList,
	LineStrip,
	TriList,
	TriStrip
};

enum ECaps
{
	Caps_VSTexFiltering_Linear
};

}

#endif

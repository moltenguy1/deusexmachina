#pragma once
#ifndef __DEM_L1_RENDER_H__
#define __DEM_L1_RENDER_H__

#include <StdDEM.h>

// Render system definitions and forward declarations

namespace Render
{

enum EClearFlag
{
	Clear_Color		= 0x01,
	Clear_Depth		= 0x02,
	Clear_Stencil	= 0x04,
	Clear_All		= (Clear_Color | Clear_Depth | Clear_Stencil)
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
	Caps_VSTex_L16,				// Unsigned short 16-bit texture as a vertex texture
	Caps_VSTexFiltering_Linear	// Bilinear min & mag filtering for vertex textures
};

}

#endif

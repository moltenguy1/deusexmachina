#pragma once
#ifndef __DEM_L1_VIDEO_PLAYER_H__
#define __DEM_L1_VIDEO_PLAYER_H__

#include <util/nstring.h>

// An abstract player for videos

class nTexture2;

namespace Video
{

class CVideoPlayer
{
protected:

	bool		_IsOpen;
	nString		FileName;
	nTexture2*	pTexture;

	uint		videoWidth;
	uint		videoHeight;
	float		videoFpS;
	uint		videoFrameCount;

public:

	//!!!TO BOOL!
	enum LoopType
	{
		Clamp = 0,
		Repeat,
	};
	bool        DoTextureUpdate;
	LoopType    loopType;

	CVideoPlayer();
	virtual ~CVideoPlayer() { n_assert(!_IsOpen); }

	virtual bool	Open();
	virtual void	Close();
	bool			IsOpen() const { return _IsOpen; }

	virtual void	Rewind() = 0;
	virtual void	DecodeNextFrame() = 0;
	virtual void	Decode(nTime DeltaTime) = 0;

	uint			GetWidth() const { return videoWidth; }
	uint			GetHeight() const { return videoHeight; }
	float			GetFpS() const { return videoFpS; }
	uint			GetFrameCount() const { return videoFrameCount; }
	void			SetFilename(const nString& _FileName);
	const nString&	GetFilename() const { return FileName; }
	void			SetTexture(nTexture2* pTex) { pTexture = pTex; }
	nTexture2*		GetTexture() const { return pTexture; }
};

}

#endif


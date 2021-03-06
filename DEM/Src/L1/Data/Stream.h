#pragma once
#ifndef __DEM_L1_STREAM_H__
#define __DEM_L1_STREAM_H__

#include <Data/Flags.h>
#include <kernel/ntypes.h>

// Base stream interface for byte sequence access.
// Partially based on Nebula 3 (c) IO::Stream class

namespace Data
{

enum EStreamAccessMode
{
	SAM_READ		= 0x01,
	SAM_WRITE		= 0x02,
	SAM_APPEND		= 0x04,
	SAM_READWRITE	= SAM_READ | SAM_WRITE
};

enum EStreamAccessPattern
{
	SAP_DEFAULT,	// Stream must set it's own preference internally if this flag is provided in Open
	SAP_RANDOM,
	SAP_SEQUENTIAL
};

enum ESeekOrigin
{
	SSO_BEGIN,
	SSO_CURRENT,
	SSO_END
};

class CStream //???refcounted?
{
public:

protected:

	enum
	{
		IS_OPEN		= 0x01,
		IS_MAPPED	= 0x02
	};

	EStreamAccessMode		AccessMode;
	EStreamAccessPattern	AccessPattern;
	CFlags					Flags;

public:

	//CStream();
	virtual ~CStream() { n_assert(!IsOpen()); }

	virtual bool	Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT) = 0;
	virtual void	Close() = 0;
	virtual DWORD	Read(void* pData, DWORD Size) = 0;
	virtual DWORD	Write(const void* pData, DWORD Size) = 0;
	virtual bool	Seek(int Offset, ESeekOrigin Origin) = 0;
	virtual void	Flush() = 0;
	virtual void*	Map() { n_assert(!IsMapped()); return NULL; }
	virtual void	Unmap() {}

	bool			IsOpen() const { return Flags.Is(IS_OPEN); }
	bool			IsMapped() const { return Flags.Is(IS_MAPPED); }
	virtual DWORD	GetSize() const = 0;
	virtual DWORD	GetPosition() const = 0;
	virtual bool	IsEOF() const = 0;
	virtual bool	CanRead() const { FAIL; }
	virtual bool	CanWrite() const { FAIL; }
	virtual bool	CanSeek() const { FAIL; }
	virtual bool	CanBeMapped() const { FAIL; }

	// For simple types (integrated or struct). Use stream writers for things like nString etc.
	template<class T>
	bool			Put(const T& Value) { return Write(&Value, sizeof(T)) == sizeof(T); }
	template<class T>
	bool			Get(T& Value) { return Read(&Value, sizeof(T)) == sizeof(T); }
	template<class T>
	T				Get() { T Value; Read(&Value, sizeof(T)); return Value; }
};

}

#endif
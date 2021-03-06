#pragma once
#ifndef __DEM_L1_POOL_H__
#define __DEM_L1_POOL_H__

//!!!TEST IT!

// Only for n_assert
#include <kernel/ndebug.h>
#include <kernel/ntypes.h>

#include <StdDEM.h>

// Simple pool for fast allocation of multiple frequently used objects, typically small.

//!!!TODO: use MaxChunks!

template <class T, DWORD ChunkSize = 128, DWORD MaxChunks = 64>
class CPool
{
protected:

	template <class T>
	struct CRecord
	{
		union
		{
			//T			Object;
			char		Object[sizeof(T)];
			CRecord<T>*	Next;
		};

		operator T*() { return (T*)Object; } 
	};

	template <class T>
	struct CChunkNode
	{
		CChunkNode<T>*	Next;
		CRecord<T>*		ChunkRecords;
		~CChunkNode() { if (Next) delete Next; delete[] ChunkRecords; }
	};

	CChunkNode<T>*	Chunks;
	CRecord<T>*		FreeRecords;

#ifdef _DEBUG
	DWORD			AllocationsCount;
	DWORD			ReleasesCount;
#endif

	T* AllocRecord();

public:

	CPool();
	~CPool() { Clear(); }

	T*		Construct();
	T*		CopyConstruct(const T& Other);
	void	Destroy(T* Handle);

	//T& Get(int idx);
	//const T& Get(int idx) const;
	//T       &operator []    (int idx) { return Get(idx); }
	//const T &operator []    (int idx) const { return Get(idx); }
	//bool     IsIndexValid   (int idx) const;

	//!!!assert number of allocations == number of releases! all living references will become invalid
	void Clear();
};

template<class T, DWORD ChunkSize, DWORD MaxChunks>
CPool<T, ChunkSize, MaxChunks>::CPool():
#ifdef _DEBUG
	AllocationsCount(0),
	ReleasesCount(0),
#endif
	Chunks(NULL),
	FreeRecords(NULL)
{
	n_assert(ChunkSize > 0);
}
//---------------------------------------------------------------------

template<class T, DWORD ChunkSize, DWORD MaxChunks>
T* CPool<T, ChunkSize, MaxChunks>::AllocRecord()
{
	T* AllocatedRec(NULL);

	if (FreeRecords)
	{
		AllocatedRec = *FreeRecords;
		FreeRecords = FreeRecords->Next;
	}
	else
	{
		CChunkNode<T>* NewChunk = new CChunkNode<T>();
		NewChunk->ChunkRecords = new CRecord<T>[ChunkSize];
		NewChunk->Next = Chunks;
		Chunks = NewChunk;

		CRecord<T>* Curr = NewChunk->ChunkRecords;
		CRecord<T>*	End = NewChunk->ChunkRecords + ChunkSize - 2;
		if (ChunkSize > 1)
		{
			Curr->Next = FreeRecords;
			for (; Curr < End; Curr++)
				(Curr + 1)->Next = Curr;
			FreeRecords = End;
		}
		AllocatedRec = *(End + 1);
	}

#ifdef _DEBUG
	AllocationsCount++;
#endif

	//???need?
	n_assert(AllocatedRec);

	return AllocatedRec;
}
//---------------------------------------------------------------------

template<class T, DWORD ChunkSize, DWORD MaxChunks>
inline T* CPool<T, ChunkSize, MaxChunks>::Construct()
{
	T* AllocatedRec = AllocRecord();
	n_placement_new(AllocatedRec, T);
	return AllocatedRec;
}
//---------------------------------------------------------------------

template<class T, DWORD ChunkSize, DWORD MaxChunks>
inline T* CPool<T, ChunkSize, MaxChunks>::CopyConstruct(const T& Other)
{
	T* AllocatedRec = AllocRecord();
	n_placement_new(AllocatedRec, T)(Other);
	return AllocatedRec;
}
//---------------------------------------------------------------------

template<class T, DWORD ChunkSize, DWORD MaxChunks>
inline void CPool<T, ChunkSize, MaxChunks>::Destroy(T* Handle)
{
#ifdef _DEBUG
	ReleasesCount++;
#endif

	//!!!assert is valid pool element - for dbg!
	Handle->~T();
	((CRecord<T>*)Handle)->Next = FreeRecords;
	FreeRecords = (CRecord<T>*)Handle;
}
//---------------------------------------------------------------------

template<class T, DWORD ChunkSize, DWORD MaxChunks>
inline void CPool<T, ChunkSize, MaxChunks>::Clear()
{
#ifdef _DEBUG
	if (AllocationsCount != ReleasesCount)
		n_error("Pool reports %d allocations and %d releases", AllocationsCount, ReleasesCount);
#endif

	if (Chunks) delete Chunks;
}
//---------------------------------------------------------------------

#endif

#pragma once
#ifndef __DEM_L1_HASH_TABLE_H__
#define __DEM_L1_HASH_TABLE_H__

#include <StdDEM.h>
#include <util/nfixedarray.h>
#include <util/narray.h>
#include <util/PairT.h>
#include <util/Hash.h>

// Hash table that uses sorted arrays as chains

//???write Grow()?

template<class TKey, class TVal>
class CHashTable
{
protected:

	typedef CPairT<TKey, TVal> CPair;
	typedef nArray<CPair> CChain;

	nFixedArray<CChain>	Chains;
	int					Count;

public:

	CHashTable(): Chains(128), Count(0) {}
    CHashTable(int Capacity): Chains(Capacity), Count(0) {}
	CHashTable(const CHashTable<TKey, TVal>& Other): Chains(Other.Chains), Count(Other.Count) {}

    void	Add(const CPairT<TKey, TVal>& Pair);
	void	Add(const TKey& Key, const TVal& value) { Add(CPair(Key, value)); }
    bool	Erase(const TKey& Key);
	void	Clear();
    bool	Contains(const TKey& Key) const;
	bool	Get(const TKey& Key, TVal& Value) const;
	TVal*	Get(const TKey& Key) const;
    void	CopyToArray(nArray<CPairT<TKey, TVal>>& OutData) const;

	int		Size() const { return Count; }
	int		Capacity() const { return Chains.Size(); }
	bool	IsEmpty() const { return !Count; }

	void	operator =(const CHashTable<TKey, TVal>& Other) { if (this != &Other) { Chains = Other.Chains; Count = Other.Count; } }
	TVal&	operator [](const TKey& Key) const { TVal* pVal = Get(Key); n_assert(pVal); return *pVal; }
};

template<class TKey, class TVal>
void CHashTable<TKey, TVal>::Add(const CPairT<TKey, TVal>& Pair)
{
	CChain& Chain = Chains[Hash(Pair.GetKey()) % Chains.Size()];
	n_assert(!Count || Chain.BinarySearchIndex(Pair.GetKey()) == INVALID_INDEX);
	Chain.InsertSorted(Pair);
	++Count;
}
//---------------------------------------------------------------------

// Returns true if element war really erased
template<class TKey, class TVal>
bool CHashTable<TKey, TVal>::Erase(const TKey& Key)
{
	if (!Count) FAIL;
	CChain& Chain = Chains[Hash(Key) % Chains.Size()];
	int ElmIdx = Chain.BinarySearchIndex(Key);
	if (ElmIdx == INVALID_INDEX) FAIL;
	Chain.Erase(ElmIdx);
	--Count;
	OK;
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
void CHashTable<TKey, TVal>::Clear()
{
	for (int i = 0; i < Chains.Size(); i++)
		Chains[i].Clear();
	Count = 0;
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
inline bool CHashTable<TKey, TVal>::Contains(const TKey& Key) const
{
	return Count && (Chains[Hash(Key) % Chains.Size()].BinarySearchIndex(Key) != INVALID_INDEX);
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
bool CHashTable<TKey, TVal>::Get(const TKey& Key, TVal& Value) const
{
	TVal* pVal = Get(Key);
	if (!pVal) FAIL;
	Value = *pVal;
	OK;
}
//---------------------------------------------------------------------

template<class TKey, class TVal>
TVal* CHashTable<TKey, TVal>::Get(const TKey& Key) const
{
	if (Count > 0)
	{
		CChain& Chain = Chains[Hash(Key) % Chains.Size()];
		if (Chain.Size() == 1)
		{
			if (Chain[0].GetKey() == Key) return &Chain[0].GetValue();
		}
		else if (Chain.Size() > 1)
		{
			int ElmIdx = Chain.BinarySearchIndex(Key);
			if (ElmIdx != INVALID_INDEX) return &Chain[ElmIdx].GetValue();
		}
	}
	return NULL;
}
//---------------------------------------------------------------------

//!!!
// Too slow. Need iterator.
template<class TKey, class TVal>
void CHashTable<TKey, TVal>::CopyToArray(nArray<CPairT<TKey, TVal>>& OutData) const
{
	for (int i = 0; i < Chains.Size(); i++)
		OutData.AppendArray(Chains[i]);
}
//---------------------------------------------------------------------

#endif
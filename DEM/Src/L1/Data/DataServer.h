#pragma once
#ifndef __DEM_L1_DATA_SERVER_H__
#define __DEM_L1_DATA_SERVER_H__

#include <Data/DataScheme.h>
#include <Core/Singleton.h>
#include <Data/HashMap.h>

// Data server manages descs and data serialization schemes

//???HRD/PRM - use loader objects or functions instead of server method?

namespace Data
{
typedef Ptr<class CParams> PParams;
typedef Ptr<class CXMLDocument> PXMLDocument;

#define DataSrv Data::CDataServer::Instance()

class CDataServer: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CDataServer);

private:

	CHashMap<PParams>					HRDCache; //!!!need better hashmap with Clear, Find etc!
	//!!!Desc cache!
	CDict<CStrID, PDataScheme>	DataSchemes;

public:

	CDataServer();
	~CDataServer() { __DestructSingleton; }

	//???to resource managers? the same cache can br implemented. or special class C...Cache?
	PParams			LoadHRD(const CString& FileName, bool Cache = true);
	PParams			ReloadHRD(const CString& FileName, bool Cache = true);	// Force reloading from file
	void			SaveHRD(const CString& FileName, const CParams* pContent);
	//???void			UnloadHRD(CParams* Data);
	void			UnloadHRD(const CString& FileName);
	//void			ClearHRDCache() { HRDCache. }

	PParams			LoadPRM(const CString& FileName, bool Cache = true);
	PParams			ReloadPRM(const CString& FileName, bool Cache = true);	// Force reloading from file
	bool			SavePRM(const CString& FileName, const CParams* pContent);
	
	PXMLDocument	LoadXML(const CString& FileName); //, bool Cache = true);

	bool			LoadDesc(PParams& Out, const CString& Context, const CString& Name, bool Cache = true);

	bool			LoadDataSchemes(const CString& FileName);
	CDataScheme*	GetDataScheme(CStrID ID);
};

inline CDataScheme* CDataServer::GetDataScheme(CStrID ID)
{
	int Idx = DataSchemes.FindIndex(ID);
	return Idx != INVALID_INDEX ? DataSchemes.ValueAt(Idx) : NULL;
}
//---------------------------------------------------------------------

}

#endif

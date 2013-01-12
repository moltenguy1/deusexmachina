#include "../StdAPI.h"
#include <Loading/EntityFactory.h>
#include "../App/CIDEApp.h"
#include <DB/DBServer.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/DataServer.h>

using namespace App;

API int Categories_GetCount(CIDEAppHandle Handle)
{
	DeclareCIDEApp(Handle);
	return EntityFct->GetCategoryCount();
}
//---------------------------------------------------------------------

API void Categories_GetName(CIDEAppHandle Handle, int Idx, char* OutCatName)
{
	DeclareCIDEApp(Handle);
	const char* Res = EntityFct->GetCategoryName(Idx);
	strcpy_s(OutCatName, 255, Res);
}
//---------------------------------------------------------------------

API int Categories_GetTplAttrCount(int Idx)
{
	DB::PDataset DS = EntityFct->GetCategory(Idx).TplDataset;
	return DS.isvalid() ? DS->GetValueTable()->GetNumColumns() : 0;
}
//---------------------------------------------------------------------

API int Categories_GetInstAttrCount(CIDEAppHandle Handle, int Idx)
{
	DeclareCIDEApp(Handle);
	DB::PDataset DS = EntityFct->GetCategory(Idx).InstDataset;
	return DS.isvalid() ? DS->GetValueTable()->GetNumColumns() : 0;
}
//---------------------------------------------------------------------

API void Categories_GetAttrID(CIDEAppHandle Handle, int CatIdx, int AttrIdx, int* Type, int* AttrID, bool* IsReadWrite, char* OutName)
{
	DeclareCIDEApp(Handle);
	DB::PDataset DS = EntityFct->GetCategory(CatIdx).InstDataset;
	const char* Res = "";
	if (DS.isvalid())
	{
		DB::CAttrID ID = DS->GetValueTable()->GetColumnID(AttrIdx);
		*Type = ID->GetType()->GetID();
		*AttrID = (int)ID;
		*IsReadWrite = (ID->GetAccessMode() == DB::ReadWrite);
		Res = ID->GetName().CStr();
	}
	strcpy_s(OutName, 255, Res);
}
//---------------------------------------------------------------------

API const char* Categories_GetAttrIDByName(const char* Name, int* Type, int* AttrID, bool* IsReadWrite)
{
	DB::CAttrID ID = DBSrv->FindAttrID(Name);
	if (ID)
	{
		*Type = ID->GetType()->GetID();
		*AttrID = (int)ID;
		*IsReadWrite = (ID->GetAccessMode() == DB::ReadWrite);
		return ID->GetName().CStr();
	}
	return "";
}
//---------------------------------------------------------------------

API bool Categories_ParseAttrDescs(CIDEAppHandle Handle, const char* FileName)
{
	DeclareCIDEApp(Handle);
	CIDEApp->AttrDescs = DataSrv->LoadHRD(FileName, false);
	return CIDEApp->AttrDescs.isvalid();
}
//---------------------------------------------------------------------

API int Categories_GetAttrDescCount(CIDEAppHandle Handle)
{
	DeclareCIDEApp(Handle);
	Data::PParams AttrDescs = CIDEApp->AttrDescs;
	if(!AttrDescs.isvalid())
		return 0;
	return AttrDescs->GetCount();
}
//---------------------------------------------------------------------

API const char* Categories_GetAttrDesc(CIDEAppHandle Handle, int Idx, char* Cat, char* Desc, char* ResFilter,
									   bool* ReadOnly, bool* ShowInList, bool* InstanceOnly)
{
	DeclareCIDEApp(Handle);
	const Data::CParam& DescParam = CIDEApp->AttrDescs->Get(Idx);
	Data::PParams AttrDesc = DescParam.GetValue<Data::PParams>();
	strcpy_s(Cat, 255, AttrDesc->Get<nString>(CStrID("Cat"), NULL).Get());
	strcpy_s(Desc, 1023, AttrDesc->Get<nString>(CStrID("Desc"), NULL).Get());
	strcpy_s(ResFilter, 1023, AttrDesc->Get<nString>(CStrID("ResourceFilter"), NULL).Get());
	*ReadOnly = AttrDesc->Get<bool>(CStrID("ReadOnly"), false);
	*ShowInList = AttrDesc->Get<bool>(CStrID("ShowInList"), true);
	*InstanceOnly = AttrDesc->Get<bool>(CStrID("InstanceOnly"), false);
	return DescParam.GetName().CStr();
}
//---------------------------------------------------------------------

API int Categories_GetTemplateCount(const char* CatName)
{
	return EntityFct->GetTemplateCount(CStrID(CatName));
}
//---------------------------------------------------------------------

API const char* Categories_GetTemplateID(const char* CatName, int Idx)
{
	return EntityFct->GetTemplateID(CStrID(CatName), Idx);
}
//---------------------------------------------------------------------

static CStrID NewCatName;
static Data::PParams NewCat;
static Data::PDataArray NewCatProps;

API void Categories_BeginCreate(const char* Name, const char* CppClass, const char* TplTable, const char* InstTable)
{
	NewCat = n_new(Data::CParams);
	NewCatName = CStrID(Name);
	NewCat->Set(CStrID("CppClass"), nString(CppClass));
	nString TplTableStr = TplTable;
	if (TplTableStr != nString("Tpl") + Name)
		NewCat->Set(CStrID("TplTableName"), TplTableStr);
	nString InstTableStr = InstTable;
	if (InstTableStr != nString("Inst") + Name)
		NewCat->Set(CStrID("InstTableName"), InstTableStr);
	NewCatProps = n_new(Data::CDataArray);
	NewCat->Set(CStrID("Props"), NewCatProps);
}
//---------------------------------------------------------------------

API void Categories_AddProperty(const char* Prop)
{
	NewCatProps->Append(nString(Prop));
}
//---------------------------------------------------------------------

API int Categories_EndCreate()
{
	EntityFct->CreateNewEntityCat(NewCatName, NewCat, true); //!!!false if no level (even empty one) loaded!
	NewCatProps = NULL;
	NewCat = NULL;
	int CatCount = EntityFct->GetCategoryCount();
	for (int i = 0; i < CatCount; i++)
		if (CStrID(EntityFct->GetCategoryName(i)) == NewCatName) return i;
	return INVALID_INDEX;
}
//---------------------------------------------------------------------

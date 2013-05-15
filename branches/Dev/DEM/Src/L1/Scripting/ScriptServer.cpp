#include "ScriptServer.h"

#include "ScriptObject.h"
#include <Events/EventManager.h>
#include <Data/DataArray.h>
#include <Data/DataServer.h>
#include <IO/IOServer.h>
#include <DB/Database.h>

extern const nString StrLuaObjects;

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

#define INITIAL_OBJ_TABLE_SIZE	8
#define TBL_CLASSES				"Classes"

namespace Scripting
{
__ImplementClassNoFactory(Scripting::CScriptServer, Core::CRefCounted);
__ImplementSingleton(CScriptServer);

CScriptServer::CScriptServer(): CurrClass(NULL), CurrObj(NULL)
{
	__ConstructSingleton;

	l = luaL_newstate();
	n_assert2(l, "Can't create main Lua interpreter");

	luaL_openlibs(l);

	IOSrv->SetAssign("scripts", "game:scripts");
	IOSrv->SetAssign("classes", "game:scripts/classes");

	// Create base class for scripted objects

	// 'Classes' table
	lua_createtable(l, 0, 16);

	// 'CScriptObject' table
	lua_createtable(l, 0, 4);

	// Create top-level index table to access global namespace
	lua_createtable(l, 0, 1);
	lua_getglobal(l, "_G");
	lua_setfield(l, -2, "__index");
	lua_setmetatable(l, -2);

	ExportCFunction("__index", CScriptObject_Index);
	ExportCFunction("__newindex", CScriptObject_NewIndex);
	ExportCFunction("SubscribeEvent", CScriptObject_SubscribeEvent);
	ExportCFunction("UnsubscribeEvent", CScriptObject_UnsubscribeEvent);
	lua_setfield(l, -2, "CScriptObject");

	lua_setglobal(l, TBL_CLASSES);

	// Unsubscribed in destructor automatically
	SUBSCRIBE_PEVENT(OnSaveBefore, CScriptServer, OnSaveBefore);
	SUBSCRIBE_PEVENT(OnSaveAfter, CScriptServer, OnSaveAfter);
}
//---------------------------------------------------------------------

CScriptServer::~CScriptServer()
{
	if (l)
	{
		lua_close(l);
		l = NULL;
	}

	__DestructSingleton;
}
//---------------------------------------------------------------------

int CScriptServer::DataToLuaStack(const CData& Data)
{
	if (Data.IsVoid()) lua_pushnil(l);
	else if (Data.IsA<bool>()) lua_pushboolean(l, Data.GetValue<bool>());
	else if (Data.IsA<int>()) lua_pushinteger(l, Data.GetValue<int>());
	else if (Data.IsA<float>()) lua_pushnumber(l, Data.GetValue<float>());
	else if (Data.IsA<nString>()) lua_pushstring(l, Data.GetValue<nString>().CStr());
	else if (Data.IsA<CStrID>()) lua_pushstring(l, Data.GetValue<CStrID>().CStr());
	else if (Data.IsA<PVOID>()) lua_pushlightuserdata(l, Data.GetValue<PVOID>());
	else if (Data.IsA<PDataArray>())
	{
		const CDataArray& A = *Data.GetValue<PDataArray>();
		lua_createtable(l, A.GetCount(), 0);
		for (int i = 0; i < A.GetCount();)
			if (DataToLuaStack(A[i]) == 1)
				lua_rawseti(l, -2, ++i);
	}
	else if (Data.IsA<PParams>())
	{
		const CParams& P = *Data.GetValue<PParams>();
		lua_createtable(l, 0, P.GetCount());
		for (int i = 0; i < P.GetCount(); ++i)
		{
			lua_pushstring(l, P[i].GetName().CStr());
			if (DataToLuaStack(P[i].GetRawValue()) == 1) lua_rawset(l, -3);
			else lua_pop(l, 1);
		}
	}
	else
	{
		n_printf("Can't push data to Lua stack, <TYPE needs dbg string>\n");
		return 0;
	}

	return 1;
}
//---------------------------------------------------------------------

bool CScriptServer::LuaStackToData(CData& Result, int StackIdx, lua_State* l)
{
	if (!l) l = l;
	if (StackIdx < 0) StackIdx = lua_gettop(l) + StackIdx + 1;

	int Type = lua_type(l, StackIdx);
	if (Type == LUA_TNIL || Type == LUA_TNONE) Result.Clear();
	else if (Type == LUA_TBOOLEAN) Result = (lua_toboolean(l, StackIdx) != 0);
	else if (Type == LUA_TNUMBER)
	{
		double Value = lua_tonumber(l, StackIdx);
		if ((double)((int)Value) == Value) Result = (int)Value;
		else Result = (float)Value;
	}
	else if (Type == LUA_TSTRING) Result = nString(lua_tostring(l, StackIdx));
	else if (Type == LUA_TLIGHTUSERDATA) Result = lua_touserdata(l, StackIdx);
	else if (Type == LUA_TTABLE)
	{						
		int Key, MaxKey = -1;	
		
		lua_pushnil(l);
		while (lua_next(l, StackIdx))
			if (lua_isstring(l, -2))
			{
				if (MaxKey > -1)
					n_printf("CScriptServer::LuaStackToData, Warning: mixed table, int & string keys, "
							 "will convert only string ones\n");
				MaxKey = -1;
				lua_pop(l, 2);
				break;
			}
			else
			{
				Key = lua_tointeger(l, -2);
				if (Key <= 0)
				{
					n_printf("CScriptServer::LuaStackToData: Incorrect array index (Idx < 0)\n");
					lua_pop(l, 2);
					FAIL;
				}
				if (Key > MaxKey) MaxKey = Key;
				lua_pop(l, 1);
			}

		if (MaxKey > -1)
		{
			PDataArray Array = n_new(CDataArray);
			Array->Reserve(MaxKey);

			lua_pushnil(l);
			while (lua_next(l, StackIdx))
			{
				Key = lua_tointeger(l, -2) - 1;
				LuaStackToData(Array->At(Key), -1, l);
				lua_pop(l, 1);
			}
			
			Result = Array;
		}
		else
		{
			CData ParamData;
			PParams Params = n_new(CParams);
			
			lua_pushnil(l);
			while (lua_next(l, StackIdx))
				if (lua_isstring(l, -2))
				{
					LPCSTR pStr = lua_tostring(l, -2);

					// Prevent diving into possible self-references
					if (strcmp(pStr, "__index") &&
						strcmp(pStr, "__newindex") &&
						strcmp(pStr, "this"))
					{
						LuaStackToData(ParamData, -1, l);
						Params->Set(CStrID(pStr), ParamData);
					}
					
					lua_pop(l, 1);
				}
			
			Result = Params;
		}
	}
	else
	{
		//n_printf("Conversion from Lua to CData failed, unsupported Lua type %d\n", Type);
		FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

EExecStatus CScriptServer::RunScriptFile(const nString& FileName)
{
	CBuffer Buffer;
	if (!IOSrv->LoadFileToBuffer(FileName, Buffer)) return Error;
	return RunScript((LPCSTR)Buffer.GetPtr(), Buffer.GetSize());
}
//---------------------------------------------------------------------

EExecStatus CScriptServer::RunScript(LPCSTR Buffer, DWORD Length, CData* pRetVal)
{
	if (luaL_loadbuffer(l, Buffer, ((Length > -1) ? Length : strlen(Buffer)), Buffer) != 0)
	{
		n_printf("Error parsing script for ScriptSrv: %s\n", lua_tostring(l, -1));
		lua_pop(l, 1); // Error msg
	}
	else
	{
		EExecStatus Result = PerformCall(0, pRetVal, "script for ScriptSrv");
		if (Result != Error) return Result;
	}

	n_printf("Script is: %s\n", Buffer);
	return Error;
}
//---------------------------------------------------------------------

// Mainly for internal use
EExecStatus CScriptServer::PerformCall(int ArgCount, CData* pRetVal, LPCSTR pDbgName)
{
	int ResultCount = lua_gettop(l) - ArgCount - 1;

	if (lua_pcall(l, ArgCount, LUA_MULTRET, 0))
	{
		n_printf("Error running %s: %s\n", pDbgName, lua_tostring(l, -1));
		lua_pop(l, 1); // Error msg
		return Error;
	}

	ResultCount = lua_gettop(l) - ResultCount;
	//!!!coroutines can return Running!

	if (ResultCount < 1) return Success;

	if (pRetVal)
	{
		n_assert(ResultCount == 1); //!!!later mb multiple return values - data array!
		LuaStackToData(*pRetVal, -1, l);
	}

	EExecStatus Result = lua_toboolean(l, -1) ? Success : Failure;
	lua_pop(l, ResultCount);
	return Result;
}
//---------------------------------------------------------------------

bool CScriptServer::BeginClass(const nString& Name, const nString& BaseClass)
{
	n_assert2(Name.IsValid(), "Invalid class name to register");
	n_assert2(CurrClass.IsEmpty(), "Already in class registration process!");
	n_assert2(!CurrObj, "Already in mixing-in process!");

	if (ClassExists(Name.CStr())) FAIL;
	if (!ClassExists(BaseClass.CStr()) && !LoadClass(BaseClass)) FAIL;

	CurrClass = Name;

	lua_getglobal(l, TBL_CLASSES);

	// Create class table
	lua_createtable(l, 0, 4); //!!!can pass fields count as arg!

	// Setup base class
	lua_getfield(l, -2, BaseClass.CStr());
	lua_setmetatable(l, -2);

	OK;
}
//---------------------------------------------------------------------

bool CScriptServer::BeginExistingClass(LPCSTR Name)
{
	n_assert(Name);
	lua_getglobal(l, TBL_CLASSES);
	lua_getfield(l, -1, Name);
	if (!lua_istable(l, -1))
	{
		lua_pop(l, 2);
		FAIL;
	}
	CurrClass = Name;
	OK;
}
//---------------------------------------------------------------------

void CScriptServer::EndClass()
{
	n_assert(CurrClass.IsValid());
	lua_setfield(l, -2, CurrClass.CStr());
	lua_pop(l, 1); // Classes table
	CurrClass = NULL;
}
//---------------------------------------------------------------------

bool CScriptServer::BeginMixin(CScriptObject* pObj)
{
	n_assert2(pObj, "Invalid object for mixin");
	n_assert2(!CurrObj, "Already in mixing-in process!");
	n_assert2(CurrClass.IsEmpty(), "Already in class registration process!");
	CurrObj = pObj;
	return PlaceObjectOnStack(pObj->GetName().CStr(), pObj->GetTable().CStr());
}
//---------------------------------------------------------------------

void CScriptServer::EndMixin()
{
	n_assert(CurrObj);
	lua_pop(l, 1);
	CurrObj = NULL;
}
//---------------------------------------------------------------------

void CScriptServer::ExportCFunction(LPCSTR Name, lua_CFunction Function)
{
	lua_pushcfunction(l, Function);
	lua_setfield(l, -2, Name); //???rawset?
}
//---------------------------------------------------------------------

void CScriptServer::ExportIntegerConst(LPCSTR Name, int Value)
{
	lua_pushinteger(l, Value);
	lua_setfield(l, -2, Name); //???rawset?
}
//---------------------------------------------------------------------

bool CScriptServer::LoadClass(const nString& Name)
{
	n_assert2(Name.IsValid(), "Invalid class name to register");

	//!!!use custom format for compiled class, because CBuffer is copied during read! Or solve this problem!
	PParams ClassDesc = DataSrv->LoadPRM("classes:" + Name + ".cls", false);

	if (ClassDesc.IsValid() &&
		BeginClass(Name, ClassDesc->Get<nString>(CStrID("Base"), "CScriptObject")))
	{
		// Here we don't know C++ class, because it's an object, not a Lua class property.
		// So, all classes use default __index & __newindex.
		ExportCFunction("__index", CScriptObject_Index);
		ExportCFunction("__newindex", CScriptObject_NewIndex);

		const char* pData = NULL;
		DWORD Size = 0;

		CParam* pCodePrm;
		if (ClassDesc->Get(pCodePrm, CStrID("Code")))
		{
			if (pCodePrm->IsA<CBuffer>())
			{
				const CBuffer& Code = pCodePrm->GetValue<CBuffer>();
				pData = (const char*)Code.GetPtr();
				Size = Code.GetSize();
			}
			else if (pCodePrm->IsA<nString>())
			{
				const nString& Code = pCodePrm->GetValue<nString>();
				pData = Code.CStr();
				Size = Code.Length();
			}
		}

		if (pData && Size)
		{
			if (luaL_loadbuffer(l, pData, Size, Name.CStr()) != 0)
			{
				n_printf("Error parsing script for class %s: %s\n", Name.CStr(), lua_tostring(l, -1));
				if (pCodePrm->IsA<nString>()) n_printf("Script is: %s\n", pData);
				lua_pop(l, 2);
				FAIL;
			}
			
			lua_pushvalue(l, -2);
			lua_setfenv(l, -2);

			if (lua_pcall(l, 0, 0, 0))
			{
				n_printf("Error running script for class %s: %s\n", Name.CStr(), lua_tostring(l, -1));
				if (pCodePrm->IsA<nString>()) n_printf("Script is: %s\n", pData);
				lua_pop(l, 2); // Error msg, class table
				FAIL;
			}
		}
		
		EndClass();
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CScriptServer::ClassExists(LPCSTR Name)
{
	lua_getglobal(l, TBL_CLASSES);
	lua_getfield(l, -1, Name);
	bool Result = lua_istable(l, -1);
	lua_pop(l, 2);
	return Result;
}
//---------------------------------------------------------------------

bool CScriptServer::CreateObject(CScriptObject& Obj, LPCSTR LuaClassName)
{
	n_assert(Obj.Name.IsValid() && Obj.Table != TBL_CLASSES && !ObjectExists(Obj.Name.CStr(), Obj.Table.CStr()));
	n_assert(LuaClassName && *LuaClassName && (ClassExists(LuaClassName) || LoadClass(LuaClassName)));

	// Create object table
	lua_createtable(l, 0, 4);

	// Setup class of this object
	lua_getglobal(l, TBL_CLASSES);
	lua_getfield(l, -1, LuaClassName);
	n_assert2(lua_istable(l, -1), "Requested Lua class does not exist");
	lua_setmetatable(l, -3);
	lua_pop(l, 1);

	// Setup object's system fields
	lua_pushstring(l, "cpp_ptr");
	lua_pushlightuserdata(l, &Obj);
	lua_rawset(l, -3);
	lua_pushstring(l, "this");
	lua_pushvalue(l, -2);
	lua_rawset(l, -3);

	// Call constructor //!!!call also parent constructors!
	lua_getfield(l, -1, LuaClassName); //???rename to OnCreate / Construct?
	if (lua_isfunction(l, -1)) 
	{
		lua_pushvalue(l, -2);
		lua_setfenv(l, -2);
		if (lua_pcall(l, 0, 0, 0))
		{
			n_printf("Error running %s class constructor for %s: %s\n",
				LuaClassName, Obj.Name.CStr(), lua_tostring(l, -1));
			lua_pop(l, 2);
			FAIL;
		}
	}
	else lua_pop(l, 1);

	// Setup containing table if needed and add new object
	if (Obj.Table.IsValid())
	{
		// Get table that contains object. Now search only in globals.
		lua_getglobal(l, Obj.Table.CStr());
		if (lua_isnil(l, -1))
		{
			// Create required table if !exist. Now adds only to globals.
			lua_pop(l, 1);
			lua_createtable(l, 0, INITIAL_OBJ_TABLE_SIZE);
			lua_pushvalue(l, -1);
			lua_setglobal(l, Obj.Table.CStr());
		}
		else if (!lua_istable(l, -1))
		{
			//???assert?
			n_printf("Error: table name \"%s\" is used by other non-table object\n", Obj.Table.CStr());
			lua_pop(l, 2);
			FAIL;
		}

		lua_pushvalue(l, -2);
		lua_setfield(l, -2, Obj.Name.CStr());
		lua_pop(l, 2);
	}
	else lua_setglobal(l, Obj.Name.CStr());

	OK;
}
//---------------------------------------------------------------------

// Places object to -1 and optionally object's containing table to -2
bool CScriptServer::PlaceObjectOnStack(LPCSTR Name, LPCSTR Table)
{
	if (!l) FAIL;

	//???!!!cache what object is currently on stack? if needed, return immediately, if new, remove old before

	int TableIdx;
	if (Table && *Table)
	{
		// Get table that contains object. Now search only in globals.
		lua_getglobal(l, Table);
		if (!lua_istable(l, -1))
		{
			n_printf("Error: table \"%s\" containing script object \"%s\" not found\n", Table, Name);
			lua_pop(l, 1);
			FAIL;
		}
		TableIdx = -1;
	}
	else TableIdx = LUA_GLOBALSINDEX;

	lua_getfield(l, TableIdx, Name);
	if (lua_istable(l, -1)) OK;
	else
	{
		n_printf("Error: script object \"%s\" not found in table \"%s\"\n",
			Name, (TableIdx == LUA_GLOBALSINDEX) ? "_Globals" : Table);
		lua_pop(l, (TableIdx == LUA_GLOBALSINDEX) ? 1 : 2);
		FAIL;
	}
}
//---------------------------------------------------------------------

// Places any named Lua var on the top of the stack
bool CScriptServer::PlaceOnStack(LPCSTR pFullName)
{
	if (!l || !pFullName) FAIL;

	char pBuffer[512];
	const char* pSrcCurr = pFullName;
	char* pDestCurr = pBuffer;
	const char* pDestEnd = pBuffer + 511;

	lua_getglobal(l, "_G");

	while (true)
	{
		if (*pSrcCurr == '.' || !*pSrcCurr)
		{
			if (pDestCurr - pBuffer > 1)
			{
				*pDestCurr = 0;
				lua_getfield(l, -1, pBuffer);
				if (lua_isnil(l, -1) || (!lua_istable(l, -1) && *pSrcCurr == '.'))
				{
					lua_pop(l, 2); // Remove parent table and value received
					FAIL;
				}
				else lua_remove(l, -2); // Remove parent table
			}
			pDestCurr = pBuffer;

			if (*pSrcCurr == '.') ++pSrcCurr;
			if (!*pSrcCurr) OK;
		}

		*pDestCurr++ = *pSrcCurr++;
		n_assert2(pDestCurr < pDestEnd, "Lua name exceeds 512 characters!\n");
	}
}
//---------------------------------------------------------------------

void CScriptServer::RemoveObject(LPCSTR Name, LPCSTR Table)
{
	if (PlaceObjectOnStack(Name, Table))
	{
		lua_pop(l, 1);
		lua_pushnil(l);
		lua_setfield(l, (Table && *Table) ? -2 : LUA_GLOBALSINDEX, Name);
		if (Table && *Table) lua_pop(l, 1);
	}
}
//---------------------------------------------------------------------

bool CScriptServer::ObjectExists(LPCSTR Name, LPCSTR Table)
{
	if (!PlaceObjectOnStack(Name, Table)) FAIL;
	lua_pop(l, (Table && *Table) ? 2 : 1);
	OK;
}
//---------------------------------------------------------------------

bool CScriptServer::OnSaveBefore(const Events::CEventBase& Event)
{
	DB::CDatabase* pDB = (DB::CDatabase*)((const Events::CEvent&)Event).Params->Get<PVOID>(CStrID("DB"));
	
	DB::PTable Tbl;
	int TableIdx = pDB->FindTableIndex(StrLuaObjects);
	if (TableIdx == INVALID_INDEX)
	{
		Tbl = DB::CTable::Create();
		Tbl->SetName(StrLuaObjects);
		Tbl->AddColumn(DB::CColumn(Attr::LuaObjName, DB::CColumn::Primary));
		Tbl->AddColumn(DB::CColumn(Attr::LuaFieldName, DB::CColumn::Primary));
		Tbl->AddColumn(Attr::LuaValue);
		pDB->AddTable(Tbl);
	}
	//else Tbl = LoaderSrv->GetGameDB()->GetTableByIndex(TableIdx);

	//???!!!create reusable dataset?!

	OK;
}
//---------------------------------------------------------------------

bool CScriptServer::OnSaveAfter(const Events::CEventBase& Event)
{
	OK;
}
//---------------------------------------------------------------------

//???universalize?
bool CScriptServer::GetTableFieldsDebug(nArray<nString>& OutFields)
{
	OutFields.Clear();

	if (!l || !lua_istable(l, -1)) FAIL;

	lua_pushnil(l);
	while (lua_next(l, -2))
	{
		nString& New = *OutFields.Reserve(1);
		New = lua_tostring(l, -2);
		switch (lua_type(l, -1))
		{
			case LUA_TNIL:				New += "(nil)"; break;
			case LUA_TBOOLEAN:			New += " = "; New += lua_tostring(l, -1); New += " (bool)"; break;
			case LUA_TNUMBER:			New += " = "; New += lua_tostring(l, -1); New += " (number)"; break;
			case LUA_TSTRING:			New += " = "; New += lua_tostring(l, -1); New += " (string)"; break;
			case LUA_TTABLE:			New += "(table)"; break;
			case LUA_TFUNCTION:			New += "(function)"; break;
			case LUA_TUSERDATA:
			case LUA_TLIGHTUSERDATA:	New += "(userdata)"; break;
		}

		lua_pop(l, 1);
	}

	lua_pop(l, 1); // Remove listed table, for now forced
	OK;
}
//---------------------------------------------------------------------

} //namespace AI
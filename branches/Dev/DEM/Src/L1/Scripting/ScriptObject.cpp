#include "ScriptObject.h"

#include "ScriptServer.h"
#include "EventHandlerScript.h"
#include <Events/EventManager.h>
#include <Data/Buffer.h>
#include <IO/IOServer.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

extern const nString StrLuaObjects("LuaObjects");

namespace Scripting
{
__ImplementClass(Scripting::CScriptObject, 'SOBJ', Core::CRefCounted);

CScriptObject::~CScriptObject()
{
	//???if (Temporary && DBSaveLoadEnabled) ClearFields();? Temporary is Lua-exposed flag too.
	ScriptSrv->RemoveObject(Name.CStr(), Table.CStr());
}
//---------------------------------------------------------------------

bool CScriptObject::Init(LPCSTR LuaClassName)
{
	return ScriptSrv->CreateObject(*this, LuaClassName);
}
//---------------------------------------------------------------------

CScriptObject* CScriptObject::GetFromStack(lua_State* l, int StackIdx)
{
	if (!lua_istable(l, StackIdx))
	{
		n_message("Can't get 'this' table, may be you used '.' instead of ':' for member call\n");
		lua_settop(l, 0);
		return NULL;
	}

	lua_pushstring(l, "cpp_ptr");
	lua_rawget(l, StackIdx);
	return (CScriptObject*)lua_touserdata(l, -1);
}
//---------------------------------------------------------------------

int CScriptObject_Index(lua_State* l)
{
	// Stack: current table at 1, key at 2

	LPCSTR Key = lua_tostring(l, 2);

	//!!!use lightuserdata objects as class instances, if possible! cpp_ptr is rewritable
	//through Lua since index/newindex aren't called for existing fields
	n_assert_dbg(strcmp(Key, "cpp_ptr"));

	lua_pushstring(l, "cpp_ptr");
	lua_rawget(l, 1);
	CScriptObject* This = (CScriptObject*)lua_touserdata(l, -1);

	if (This)
	{
		if (!strcmp(Key, "name"))
		{
			lua_pushstring(l, This->GetName().CStr());
			return 1;
		}

		int Pushed = This->GetField(Key);

		if (Pushed > 0) return Pushed;
	}

	lua_getmetatable(l, 1);
	lua_getfield(l, -1, Key);
	return 1;
}
//---------------------------------------------------------------------

int CScriptObject_NewIndex(lua_State* l)
{
	// Stack: current table at 1, key at 2, value at 3

	const char* Key = lua_tostring(l, 2);

	n_assert_dbg(strcmp(Key, "cpp_ptr"));

	lua_pushstring(l, "cpp_ptr");
	lua_rawget(l, 1);
	CScriptObject* This = (CScriptObject*)lua_touserdata(l, -1);

	if (This)
	{
		if (!strcmp(Key, "name"))
		{
			n_assert(lua_isstring(l, 3));
			This->SetName(lua_tostring(l, 3));

			// Never cache the name, cause next rewrite will not call __newindex and will break object naming
			// it's not fatal if object can someway store its table instead of getting it from globals by name
			return 0;
		}

		Data::CData Data;
		if (ScriptSrv->LuaStackToData(Data, 3, l) && This->SetField(Key, Data)) return 0;
	}

	lua_pushvalue(l, 2);
	lua_pushvalue(l, 3);
	lua_rawset(l, 1);

	return 0;
}
//---------------------------------------------------------------------

int CScriptObject_SubscribeEvent(lua_State* l)
{
	// Args: ScriptObject's this table, event name, [func name = event name]

	//!!!PRIORITY!
	CScriptObject* This = CScriptObject::GetFromStack(l, 1);
	if (This) This->SubscribeEvent(CStrID(lua_tostring(l, 2)), lua_tostring(l, -2));
	return 0;
}
//---------------------------------------------------------------------

int CScriptObject_UnsubscribeEvent(lua_State* l)
{
	//args: ScriptObject's this table or nil, event name, [func name = event name]

	CScriptObject* This = CScriptObject::GetFromStack(l, 1);
	if (This) This->UnsubscribeEvent(CStrID(lua_tostring(l, 2)), lua_tostring(l, -2));
	return 0;
}
//---------------------------------------------------------------------

EExecStatus CScriptObject::LoadScriptFile(const nString& FileName)
{
	Data::CBuffer Buffer;
	if (!IOSrv->LoadFileToBuffer(FileName, Buffer) &&
		!IOSrv->LoadFileToBuffer("scripts:" + FileName + ".lua", Buffer)) return Error;
	return LoadScript((LPCSTR)Buffer.GetPtr(), Buffer.GetSize());
}
//---------------------------------------------------------------------

EExecStatus CScriptObject::LoadScript(LPCSTR Buffer, DWORD Length)
{
	lua_State* l = ScriptSrv->GetLuaState();

	if (luaL_loadbuffer(l, Buffer, Length, Buffer) != 0)
	{
		n_printf("Error parsing script for %s: %s\n", Name.CStr(), lua_tostring(l, -1));
		n_printf("Script is: %s\n", Buffer);
		lua_pop(l, 1);
		return Error;
	}

	if (!ScriptSrv->PlaceObjectOnStack(Name.CStr(), Table.CStr()))
	{
		lua_pop(l, 1);
		return Error;
	}

	if (Table.IsValid()) lua_remove(l, -2);
	lua_setfenv(l, -2);

	EExecStatus Result = RunFunctionInternal("<LOADING NEW SCRIPT>", 0, NULL);
	if (Result == Error) n_printf("Script is: %s\n", Buffer);
	return Result;
}
//---------------------------------------------------------------------

bool CScriptObject::PrepareToLuaCall(LPCSTR pFuncName) const
{
	n_assert(pFuncName);

	lua_State* l = ScriptSrv->GetLuaState();

	if (!ScriptSrv->PlaceObjectOnStack(Name.CStr(), Table.CStr())) FAIL;

	if (Table.IsValid()) lua_remove(l, -2);

	lua_getfield(l, -1, pFuncName);
	if (!lua_isfunction(l, -1)) 
	{
		n_printf("Error: function \"%s\" not found in script object \"%s\"\n", pFuncName, Name.CStr());
		lua_pop(l, 2);
		FAIL;
	}

	// Set env for the case when function is inherited from metatable
	lua_pushvalue(l, -2);
	lua_setfenv(l, -2);

	OK;
}
//---------------------------------------------------------------------

EExecStatus CScriptObject::RunFunctionInternal(LPCSTR pFuncName, int ArgCount, Data::CData* pRetVal) const
{
	EExecStatus Result = ScriptSrv->PerformCall(ArgCount, pRetVal, (Name + "." + pFuncName).CStr());
	if (Result == Error) lua_pop(ScriptSrv->GetLuaState(), 1); // Object itself
	return Result;
}
//---------------------------------------------------------------------

EExecStatus CScriptObject::RunFunction(LPCSTR pFuncName, LPCSTR LuaArg, Data::CData* pRetVal) const
{
	if (!PrepareToLuaCall(pFuncName)) return Error;
	lua_getglobal(ScriptSrv->GetLuaState(), LuaArg); //???only globals are allowed? //???assert nil?
	return RunFunctionInternal(pFuncName, 1, pRetVal);
}
//---------------------------------------------------------------------

EExecStatus CScriptObject::RunFunctionOneArg(LPCSTR pFuncName, const Data::CData& Arg, Data::CData* pRetVal) const
{
	if (!PrepareToLuaCall(pFuncName)) return Error;
	ScriptSrv->DataToLuaStack(Arg);
	return RunFunctionInternal(pFuncName, 1, pRetVal);
}
//---------------------------------------------------------------------

bool CScriptObject::SubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, Events::CEventDispatcher* pDisp, ushort Priority)
{
	Events::PSub Sub = pDisp->AddHandler(EventID, n_new(Events::CEventHandlerScript)(this, HandlerFuncName, Priority));
	if (!Sub.IsValid()) FAIL;
	Subscriptions.Append(Sub);
	OK;
}
//---------------------------------------------------------------------

void CScriptObject::UnsubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, const Events::CEventDispatcher* pDisp)
{
	for (int i = 0; i < Subscriptions.GetCount(); i++)
	{
		Events::PSub CurrSub = Subscriptions[i];
		if (CurrSub->GetEvent() == EventID && CurrSub->GetDispatcher() == pDisp &&
			((Events::CEventHandlerScript*)CurrSub->GetHandler())->GetFunc() == HandlerFuncName)
		{
			Subscriptions.EraseAt(i);
			break; //???or scan all array for duplicates?
		}
	}
}
//---------------------------------------------------------------------

bool CScriptObject::SubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName, ushort Priority)
{
	return SubscribeEvent(EventID, HandlerFuncName, EventMgr, Priority);
}
//---------------------------------------------------------------------

void CScriptObject::UnsubscribeEvent(CStrID EventID, LPCSTR HandlerFuncName)
{
	UnsubscribeEvent(EventID, HandlerFuncName, EventMgr);
}
//---------------------------------------------------------------------

void CScriptObject::SetName(const char* NewName)
{
	n_assert(NewName && *NewName);

	if (Name == NewName) return;

	if (!ScriptSrv->PlaceObjectOnStack(Name.CStr(), Table.CStr())) return;

	int TableIdx = Table.IsValid() ? -2 : LUA_GLOBALSINDEX;
	lua_State* l = ScriptSrv->GetLuaState();
	lua_setfield(l, TableIdx, NewName);
	lua_pushnil(l);
	lua_setfield(l, TableIdx, Name.CStr());
	Name = NewName;
}
//---------------------------------------------------------------------

}
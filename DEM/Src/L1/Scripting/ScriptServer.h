#pragma once
#ifndef __DEM_L1_SCRIPT_SERVER_H__
#define __DEM_L1_SCRIPT_SERVER_H__

#include <Core/RefCounted.h>
#include <StdDEM.h>
#include <Events/Events.h>

// Script server is a central point for script objects creation, registration and script running
// Should store script interpreter, loader from file etc
// Current implementation uses Lua 5.1.4 patch 4 as a scripting language

//???cpp_ptr as closure var?

struct lua_State;
typedef int (*lua_CFunction) (lua_State *L);

namespace Data
{
	class CData;
}

namespace Scripting
{

using namespace Data;

typedef Ptr<class CScriptObject> PScriptObject;

#define ScriptSrv Scripting::CScriptServer::Instance()

class CScriptServer: public Core::CRefCounted
{
	DeclareRTTI;
	__DeclareSingleton(CScriptServer);

private:

	lua_State*		l; // Yes, just 'l'. It's very convinient when writing & reading tons of Lua-related code.

	nString			CurrClass;
	CScriptObject*	CurrObj;

	DECLARE_EVENT_HANDLER(OnSaveBefore, OnSaveBefore);
	DECLARE_EVENT_HANDLER(OnSaveAfter, OnSaveAfter);

public:

	CScriptServer();
	~CScriptServer();

	lua_State*	GetLuaState() const { return l; }

	int			DataToLuaStack(const CData& Data);
	bool		LuaStackToData(CData& Result, int StackIdx, lua_State* L = NULL);

	EExecStatus	RunScriptFile(const nString& FileName);
	EExecStatus	RunScript(LPCSTR Buffer, DWORD Length = -1, CData* pRetVal = NULL);

	EExecStatus	PerformCall(int ArgCount, CData* pRetVal = NULL, LPCSTR pDbgName = "<UNKNOWN>");
	
	//EExecStatus		RunFunction(LPCSTR pFuncName, int ArgsOnStack = 0);
	//EExecStatus		RunFunction(LPCSTR pFuncName, LPCSTR LuaArg);
	//EExecStatus		RunFunction(LPCSTR pFuncName, const nArray<LPCSTR>& LuaArgs);
	//EExecStatus		RunFunction(LPCSTR pFuncName, PParams Args = NULL);

	// Class registration, Mixing-in
	bool		BeginClass(const nString& Name, const nString& BaseClass = "CScriptObject");
	bool		BeginExistingClass(LPCSTR Name);
	void		EndClass();
	bool		BeginMixin(CScriptObject* pObj);
	void		EndMixin();
	void		ExportCFunction(LPCSTR Name, lua_CFunction Function);
	void		ExportIntegerConst(LPCSTR Name, int Value);

	bool		LoadClass(const nString& Name);
	bool		ClassExists(LPCSTR Name);

	//!!!can add functions to subscribe global functions to events!
	
	bool		CreateObject(CScriptObject& Obj, LPCSTR LuaClassName = "CScriptObject");
	bool		PlaceObjectOnStack(LPCSTR Name, LPCSTR Table = NULL);
	bool		PlaceOnStack(LPCSTR pFullName);
	void		RemoveObject(LPCSTR Name, LPCSTR Table = NULL);
	bool		ObjectExists(LPCSTR Name, LPCSTR Table = NULL);

	bool		GetTableFieldsDebug(nArray<nString>& OutFields);
};

}

#endif

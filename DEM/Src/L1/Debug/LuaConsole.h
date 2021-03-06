#pragma once
#ifndef __DEM_L1_DBG_LUA_CONSOLE_H__
#define __DEM_L1_DBG_LUA_CONSOLE_H__

#include <UI/Window.h>
#include <Events/Events.h>
#include <Events/Subscription.h>
#include <Data/SimpleString.h>

// Lua console window, that allows to see engine output and to execute Lua commands.

namespace CEGUI
{
	class Editbox;
	class Listbox;
	class Scrollbar;
	class EventArgs;
}

namespace Debug
{

class CLuaConsole: public UI::CWindow
{
	DeclareRTTI;
	DeclareFactory(CLuaConsole);

protected:

	CEGUI::Editbox*				pInputLine;
	CEGUI::Listbox*				pOutputWnd;
	CEGUI::Scrollbar*			pVertScroll;

	CEGUI::Event::Connection	ConnOnShow;
	//CEGUI::Event::Connection	ConnOnChar;

	nArray<Data::CSimpleString>	CmdHistory;
	int							CmdHistoryCursor;

	void Print(LPCSTR pMsg, DWORD Color);

	bool OnShow(const CEGUI::EventArgs& e);
	bool OnChar(const CEGUI::EventArgs& e);
	bool OnCommand(const CEGUI::EventArgs& e);
	bool OnKeyDown(const CEGUI::EventArgs& e);

	DECLARE_EVENT_HANDLER(OnLogMsg, OnLogMsg);

public:

	CLuaConsole(): CmdHistoryCursor(0) {}
	//virtual ~CLuaConsole() {}

	virtual void	Init(CEGUI::Window* pWindow);
	virtual void	Term();
	void			SetInputFocus() { ((CEGUI::Window*)pInputLine)->activate(); }
};

RegisterFactory(CLuaConsole);

typedef Ptr<CLuaConsole> PLuaConsole;

}

#endif
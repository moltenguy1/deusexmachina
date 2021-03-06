#include "DebugServer.h"

#include <UI/UIServer.h>
#include <Input/InputServer.h>
#include <Events/EventManager.h>

//!!!Only for DebugBreak()!
#ifdef _DEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Dbg menu plugins:
// console - OK
// texture browser - can create (CC)
// gfx resources - not usable (NU)
// hardpoints - NU
// scene control - NU
// watcher - OK
// system info - CC
// adjust display - CC
// character control - NU
// settings mgmt - NU
// close menu - CC
// quit - OK

namespace Debug
{
ImplementRTTI(Debug::CDebugServer, Core::CRefCounted);
__ImplementSingleton(Debug::CDebugServer);

CDebugServer::CDebugServer(): UIAllowed(false)
{
	__ConstructSingleton;
	SUBSCRIBE_PEVENT(DebugBreak, CDebugServer, OnDebugBreak);
}
//---------------------------------------------------------------------

CDebugServer::~CDebugServer()
{
	UNSUBSCRIBE_EVENT(DebugBreak);
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CDebugServer::RegisterPlugin(CStrID Name, LPCSTR CppClassName, LPCSTR UIResource)
{
	CPlugin New;
	New.UIResource = UIResource;
	New.Window = (UI::CWindow*)CoreFct->Create(CppClassName);
	n_assert(New.Window.isvalid());
	//!!!call InitPlugin or write all Init code in the virtual CWindow::Init!
	Plugins.Add(Name, New);
	OK;
}
//---------------------------------------------------------------------

void CDebugServer::AllowUI(bool Allow)
{
	UIAllowed = Allow;

	if (UIAllowed)
	{
		SUBSCRIBE_PEVENT(ShowDebugConsole, CDebugServer, OnShowDebugConsole);
		SUBSCRIBE_PEVENT(ShowDebugWatcher, CDebugServer, OnShowDebugWatcher);
	}
	else
	{
		UNSUBSCRIBE_EVENT(ShowDebugConsole);
		UNSUBSCRIBE_EVENT(ShowDebugWatcher);

		for (int i = 0; i < Plugins.Size(); ++i)
			if (Plugins.ValueAtIndex(i).UIResource.IsValid())
				Plugins.ValueAtIndex(i).Window->Term();
	}
}
//---------------------------------------------------------------------

bool CDebugServer::OnDebugBreak(const Events::CEventBase& Event)
{
#ifdef _DEBUG
	DebugBreak();
#endif
	OK;
}
//---------------------------------------------------------------------

bool CDebugServer::OnShowDebugConsole(const Events::CEventBase& Event)
{
	TogglePluginWindow(CStrID("Console"));
	OK;
}
//---------------------------------------------------------------------

bool CDebugServer::OnShowDebugWatcher(const Events::CEventBase& Event)
{
	TogglePluginWindow(CStrID("Watcher"));
	OK;
}
//---------------------------------------------------------------------

void CDebugServer::TogglePluginWindow(CStrID Name)
{
	CPlugin* pPlugin = Plugins.Find(Name);
	if (!pPlugin) return;

	UI::CWindow& UI = *pPlugin->Window;
	if (UI.GetWnd() && UI.GetWnd()->getParent() == UISrv->GetRootScreen()->GetWnd())
		UI.ToggleVisibility();
	else
	{
		if (!UI.GetWnd()) 
		{
			UI.Load(pPlugin->UIResource.CStr());
			n_assert(UI.GetWnd());
		}
		if (UI.GetWnd()->getParent() != UISrv->GetRootScreen()->GetWnd())
		{
			UISrv->GetRootScreen()->GetWnd()->addChildWindow(UI.GetWnd());
			UI.Hide(); // To force OnShow event
			UI.Show();
		}
	}
	//UI.SetInputFocus();
}
//---------------------------------------------------------------------

void CDebugServer::Trigger()
{
}
//---------------------------------------------------------------------

} //namespace Debug
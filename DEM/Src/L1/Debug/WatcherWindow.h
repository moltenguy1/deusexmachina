#pragma once
#ifndef __DEM_L1_DBG_WATCHER_WINDOW_H__
#define __DEM_L1_DBG_WATCHER_WINDOW_H__

#include <UI/Window.h>
#include <Events/Events.h>
#include <Events/Subscription.h>
#include <Data/SimpleString.h>

// Nebula variable watcher with pattern-matching filter

// Can add capability to add and remove vars to the list by user

namespace CEGUI
{
	class Editbox;
	class MultiColumnList;
	class ListboxTextItem;
	class EventArgs;
}

namespace Debug
{

class CWatcherWindow: public UI::CWindow
{
	DeclareRTTI;
	DeclareFactory(CWatcherWindow);

public:

	enum EVarType
	{
		NEnv = 0,
		Lua
	};

protected:

	CEGUI::Editbox*				pPatternEdit;
	CEGUI::Editbox*				pNewWatchEdit;
	CEGUI::MultiColumnList*		pList;

	struct CWatched
	{
		EVarType				Type;
		Data::CSimpleString		VarName;
		int						RowID;
		CEGUI::ListboxTextItem*	pNameItem;
		CEGUI::ListboxTextItem*	pTypeItem;
		CEGUI::ListboxTextItem*	pValueItem;

		CWatched(): RowID(-1), pNameItem(NULL), pTypeItem(NULL), pValueItem(NULL) {}
		void Clear();
	};

	nArray<CWatched>			Watched;

	bool OnClearClick(const CEGUI::EventArgs& e);
	bool OnAddVarsClick(const CEGUI::EventArgs& e);
	bool OnNewWatchedAccept(const CEGUI::EventArgs& e);
	bool OnListKeyDown(const CEGUI::EventArgs& e);

	DECLARE_EVENT_HANDLER(OnUIUpdate, OnUIUpdate);

public:

	//CWatcherWindow() {}
	//virtual ~CWatcherWindow() {}

	virtual void	Init(CEGUI::Window* pWindow);
	virtual void	Term();
	virtual void	SetVisible(bool Visible);
	void			SetInputFocus() { ((CEGUI::Window*)pPatternEdit)->activate(); }

	void			AddWatched(EVarType Type, LPCSTR Name);
	void			AddAllVars();
};

RegisterFactory(CWatcherWindow);

typedef Ptr<CWatcherWindow> PWatcherWindow;

}

#endif
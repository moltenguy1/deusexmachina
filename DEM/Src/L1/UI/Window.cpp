#include "Window.h"

#include <gfx2/ngfxserver2.h>
#include <CEGUIWindowManager.h>

namespace UI
{
ImplementRTTI(UI::CWindow, Core::CRefCounted);
ImplementFactory(UI::CWindow);

void CWindow::Init(CEGUI::Window* pWindow)
{
	n_assert(!pWnd && pWindow);
	pWnd = pWindow;
}
//---------------------------------------------------------------------

void CWindow::Load(const nString& ResourceFile)
{
	Init(CEGUI::WindowManager::getSingleton().loadWindowLayout(ResourceFile.Get(), ""));
}
//---------------------------------------------------------------------

vector2 CWindow::GetSizeRel()
{
	if (!pWnd) return vector2::zero;

	vector2 ParentSize = GetParentBaseSize(pWnd);
	return vector2(pWnd->getWidth().asRelative(ParentSize.x), pWnd->getHeight().asRelative(ParentSize.y));
}
//---------------------------------------------------------------------

vector2 CWindow::GetParentBaseSize(CEGUI::Window* pWindow)
{
	CEGUI::Window* pWndParent = pWindow->getParent();
	if (!pWndParent)
	{
		const CDisplayMode& Disp = nGfxServer2::Instance()->GetDisplayMode();
		return vector2((float)Disp.Width, (float)Disp.Height);
	}

	vector2 GrandParentSize = GetParentBaseSize(pWndParent);
	return vector2(pWndParent->getWidth().asAbsolute(GrandParentSize.x), pWndParent->getHeight().asAbsolute(GrandParentSize.y));
}
//---------------------------------------------------------------------

}

#ifdef __WIN32__

#include "OSWindowWin32.h"

#include <Render/Events/DisplayInput.h> //???or custom listener for this?
#include <Events/EventServer.h>

//???XP or higher? put related code under define!
#include <Uxtheme.h>
#include <WindowsX.h>

//???in what header?
#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC	((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE	((USHORT) 0x02)
#endif

#define DEM_WINDOW_CLASS		"DeusExMachina::MainWindow"
#define DEM_DEFAULT_TITLE		"DeusExMachina - Untitled"
#define ACCEL_TOGGLEFULLSCREEN	1001
#define STYLE_WINDOWED			(WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE)
#define STYLE_FULLSCREEN		(WS_POPUP | WS_SYSMENU | WS_VISIBLE)
#define STYLE_CHILD				(WS_CHILD | WS_TABSTOP | WS_VISIBLE) //???need tabstop?

namespace Sys
{

COSWindowWin32::COSWindowWin32():
	WindowTitle(DEM_DEFAULT_TITLE),
	pParent(NULL),
	hInst(NULL),
	hWnd(NULL),
	hAccel(NULL),
	aWndClass(0)
{
	hInst = ::GetModuleHandle(NULL);
}
//---------------------------------------------------------------------

//!!!single-window approach! need to be rewritten to support multiple windows!
COSWindowWin32::~COSWindowWin32()
{
	if (Flags.Is(Wnd_Open)) Close();

	if (hAccel)
	{
		DestroyAcceleratorTable(hAccel);
		hAccel = NULL;
	}

	if (aWndClass)
	{
		if (!UnregisterClass((LPCSTR)aWndClass, hInst))
			Sys::Error("COSWindowWin32::CloseWindow(): UnregisterClass() failed!\n");
		aWndClass = 0;
	}
}
//---------------------------------------------------------------------

bool COSWindowWin32::Open()
{
	n_assert(!Flags.Is(Wnd_Open) && hInst && !hWnd && (!pParent || pParent->GetHWND()));

	// Send DisplayOpen event

	if (!hAccel)
	{
		ACCEL Acc[1];
		Acc[0].fVirt = FALT | FNOINVERT | FVIRTKEY;
		Acc[0].key = VK_RETURN;
		Acc[0].cmd = ACCEL_TOGGLEFULLSCREEN;
		hAccel = CreateAcceleratorTable(Acc, 1);
		n_assert(hAccel);
	}

	if (!aWndClass)
	{
		HICON hIcon = NULL;
		if (IconName.IsValid()) hIcon = ::LoadIcon(hInst, IconName.CStr());
		if (!hIcon) hIcon = ::LoadIcon(NULL, IDI_APPLICATION);

		WNDCLASSEX WndClass;
		memset(&WndClass, 0, sizeof(WndClass));
		WndClass.cbSize        = sizeof(WndClass);
		WndClass.style         = CS_DBLCLKS;
		WndClass.lpfnWndProc   = WinProc;
		WndClass.cbClsExtra    = 0;
		WndClass.cbWndExtra    = sizeof(void*);   // used to hold 'this' pointer
		WndClass.hInstance     = hInst;
		WndClass.hIcon         = hIcon;
		WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
		WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		WndClass.lpszMenuName  = NULL;
		WndClass.lpszClassName = DEM_WINDOW_CLASS;
		WndClass.hIconSm       = NULL;
		aWndClass = ::RegisterClassEx(&WndClass);
		if (!aWndClass) FAIL;
	}

	LONG WndStyle;
	RECT r;
	if (pParent)
	{
		WndStyle = STYLE_CHILD;
		::GetClientRect(pParent->GetHWND(), &r);
	}
	else
	{
		WndStyle = Flags.Is(Wnd_Fullscreen) ? STYLE_FULLSCREEN : STYLE_WINDOWED;
		r.left = Rect.Left();
		r.top = Rect.Top();
		r.right = Rect.Right();
		r.bottom = Rect.Bottom();
		::AdjustWindowRect(&r, WndStyle, FALSE);
	}

	hWnd = ::CreateWindowEx(Flags.Is(Wnd_Topmost) ? WS_EX_TOPMOST : 0,
							(LPCSTR)(DWORD_PTR)aWndClass, WindowTitle.CStr(), WndStyle,
							r.left, r.top, r.right - r.left, r.bottom - r.top,
							pParent ? pParent->GetHWND() : NULL, NULL, hInst, NULL);
	if (!hWnd) FAIL;

	::SetWindowLongPtr(hWnd, 0, (LONG)this);

	::ShowWindow(hWnd, SW_SHOWDEFAULT);

	::GetClientRect(hWnd, &r);
	Rect.X = r.left;
	Rect.Y = r.top;
	Rect.W = r.right - r.left;
	Rect.H = r.bottom - r.top;

	RAWINPUTDEVICE RawInputDevices;
	RawInputDevices.usUsagePage = HID_USAGE_PAGE_GENERIC; 
	RawInputDevices.usUsage = HID_USAGE_GENERIC_MOUSE; 
	RawInputDevices.dwFlags = RIDEV_INPUTSINK;
	RawInputDevices.hwndTarget = hWnd;

	if (::RegisterRawInputDevices(&RawInputDevices, 1, sizeof(RAWINPUTDEVICE)) == FALSE)
		Sys::Log("COSWindowWin32: High-definition (raw) mouse device registration failed!\n");

	Flags.Set(Wnd_Open);
	Flags.SetTo(Wnd_Minimized, ::IsIconic(hWnd) == TRUE);

	OK;
}
//---------------------------------------------------------------------

//!!!redesign display closing!
void COSWindowWin32::Close()
{
	n_assert(Flags.Is(Wnd_Open) && hInst);

	// Close if not already closed externally (e.g. by Alt-F4)
	if (hWnd)
	{
		::DestroyWindow(hWnd);
		hWnd = NULL;
	}

	// send DisplayClose event

	Flags.Clear(Wnd_Open);
}
//---------------------------------------------------------------------

void COSWindowWin32::Minimize()
{
	n_assert_dbg(hWnd && Flags.Is(Wnd_Open));
	if (!Flags.Is(Wnd_Minimized) && !pParent)
		::ShowWindow(hWnd, SW_MINIMIZE);
}
//---------------------------------------------------------------------

void COSWindowWin32::Restore()
{
	n_assert_dbg(hWnd && Flags.Is(Wnd_Open));
	::ShowWindow(hWnd, SW_RESTORE);
}
//---------------------------------------------------------------------

// Polls for and processes window messages. Call this message once per
// frame in your render loop. If the user clicks the window close
// button, or hits Alt-F4, an OnDisplayClose event will be sent.
void COSWindowWin32::ProcessMessages()
{
	n_assert_dbg(Flags.Is(Wnd_Open));

	// It may happen that the WinProc has already closed our window!
	if (!hWnd) return;

	// NB: we pass NULL instead of window handle to receive language switching messages
	MSG Msg;
	while (::PeekMessage(&Msg, NULL /*hWnd*/, 0, 0, PM_REMOVE))
	{
		if (hAccel && ::TranslateAccelerator(hWnd, hAccel, &Msg) != FALSE) continue;
		::TranslateMessage(&Msg);
		::DispatchMessage(&Msg);
	}
}
//---------------------------------------------------------------------

bool COSWindowWin32::SetRect(const Data::CRect& NewRect, bool FullscreenMode)
{
	if (!hWnd)
	{
		Rect = NewRect;
		OK;
	}

	UINT SWPFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS;

	LONG PrevWndStyle = (LONG)::GetWindowLongPtr(hWnd, GWL_STYLE);
	LONG NewWndStyle = pParent ? STYLE_CHILD : (FullscreenMode ? STYLE_FULLSCREEN : STYLE_WINDOWED);
	
	if (NewWndStyle != PrevWndStyle)
	{
		if (::SetWindowLongPtr(hWnd, GWL_STYLE, NewWndStyle) == 0) FAIL;
		Flags.SetTo(Wnd_Fullscreen, FullscreenMode && !pParent);

		SWPFlags |= SWP_FRAMECHANGED;
		
		// Fullscreen mode breaks theme (at least aero glass) on Win7, so restore it when returned from the fullscreen mode
		if (PrevWndStyle == STYLE_FULLSCREEN) ::SetWindowTheme(hWnd, NULL, NULL);
	}

	//???default empty rect values? zero w & h

	RECT r = { NewRect.Left(), NewRect.Top(), NewRect.Right(), NewRect.Bottom() };
	::AdjustWindowRect(&r, NewWndStyle, FALSE);

	if (::SetWindowPos(hWnd, NULL, Rect.X, Rect.Y, Rect.W, Rect.H, SWPFlags) == FALSE) FAIL;

	::GetClientRect(hWnd, &r);
	Rect.X = r.left;
	Rect.Y = r.top;
	Rect.W = r.right - r.left;
	Rect.H = r.bottom - r.top;

	OK;
}
//---------------------------------------------------------------------

void COSWindowWin32::SetTitle(const char* pTitle)
{
	WindowTitle = pTitle;
	if (hWnd) ::SetWindowText(hWnd, pTitle);
}
//---------------------------------------------------------------------

void COSWindowWin32::SetIcon(const char* pIconName)
{
	IconName = pIconName;
	if (hWnd && IconName.IsValid())
	{
		HICON hIcon = ::LoadIcon(hInst, IconName.CStr());
		if (hIcon) ::SetClassLong(hWnd, GCL_HICON, (LONG)hIcon);
	}
}
//---------------------------------------------------------------------

bool COSWindowWin32::SetTopmost(bool Topmost)
{
	if (pParent && Topmost) FAIL; //???or allow?
	if (Flags.Is(Wnd_Topmost) == Topmost) OK;

	if (hWnd)
	{
		HWND hWndInsertAfter;
		if (Topmost) hWndInsertAfter = HWND_TOPMOST;
		else if (pParent) hWndInsertAfter = pParent->GetHWND();
		else hWndInsertAfter = HWND_NOTOPMOST;
	
		if (::SetWindowPos(hWnd, hWndInsertAfter, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE) == FALSE) FAIL;
	}
	
	Flags.SetTo(Wnd_Topmost, Topmost);
	OK;
}
//---------------------------------------------------------------------

LONG WINAPI COSWindowWin32::WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	COSWindowWin32* pWnd = (COSWindowWin32*)::GetWindowLong(hWnd, 0);
	LONG Result = 0;
	if (pWnd && pWnd->HandleWindowMessage(uMsg, wParam, lParam, Result)) return Result;
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------

//!!!many messages fall back as not processed. Check it!
bool COSWindowWin32::HandleWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LONG& Result)
{
	switch (uMsg)
	{
		case WM_SYSCOMMAND:
			// Prevent moving/sizing and power loss in fullscreen mode
			if (Flags.Is(Wnd_Fullscreen))
			{
				switch (wParam)
				{
					case SC_MOVE:
					case SC_SIZE:
					case SC_MAXIMIZE:
					case SC_KEYMENU:
					case SC_MONITORPOWER:
						Result = 0; //1
						OK;
				}
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ACCEL_TOGGLEFULLSCREEN:
					EventSrv->FireEvent(CStrID("OnDisplayToggleFullscreen"));
					break;
			}
			break;

		case WM_PAINT:
			EventSrv->FireEvent(CStrID("OnDisplayPaint"));
			break;

		case WM_ERASEBKGND:
			// Prevent Windows from erasing the background
			Result = 1;
			OK;

		case WM_MOVE:
		{
			unsigned int X = (unsigned int)(short)LOWORD(lParam);
			unsigned int Y = (unsigned int)(short)HIWORD(lParam);

			if (Rect.X != X || Rect.Y != Y)
			{
				Rect.X = X;
				Rect.Y = Y;
				EventSrv->FireEvent(CStrID("OnDisplayMoved"));
			}
			break;
		}

		case WM_SIZE:
		{	
			if (wParam == SIZE_MAXHIDE || wParam == SIZE_MINIMIZED)
			{
				if (!Flags.Is(Wnd_Minimized))
				{
					Flags.Set(Wnd_Minimized);
					EventSrv->FireEvent(CStrID("OnDisplayMinimized"));
					ReleaseCapture();
				}
			}
			else
			{
				if (Flags.Is(Wnd_Minimized))
				{
					Flags.Clear(Wnd_Minimized);
					EventSrv->FireEvent(CStrID("OnDisplayRestored"));
					ReleaseCapture();
				}

				n_assert_dbg(!Flags.Is(Wnd_Fullscreen));

				unsigned int W = (unsigned int)(short)LOWORD(lParam);
				unsigned int H = (unsigned int)(short)HIWORD(lParam);

				if (Rect.W != W || Rect.H != H)
				{
					Rect.W = W;
					Rect.H = H;
					EventSrv->FireEvent(CStrID("OnDisplaySizeChanged"));
				}
			}
			
			break;
		}

		case WM_SETCURSOR:
			if (EventSrv->FireEvent(CStrID("OnDisplaySetCursor")))
			{
				Result = TRUE;
				OK;
			}
			break;

		case WM_SETFOCUS:
			EventSrv->FireEvent(CStrID("OnDisplaySetFocus"));
			ReleaseCapture();
			break;

		case WM_KILLFOCUS:
			EventSrv->FireEvent(CStrID("OnDisplayKillFocus"));
			ReleaseCapture();
			break;

		//!!!on close destroy window, on destroy post quit message if not child, or just send event to engine!
		case WM_CLOSE:
			EventSrv->FireEvent(CStrID("OnDisplayClose"));
			hWnd = NULL;
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			Event::DisplayInput Ev;
			Ev.Type = (uMsg == WM_KEYDOWN) ? Event::DisplayInput::KeyDown : Event::DisplayInput::KeyUp;
			Ev.KeyCode = (Input::EKey)((uchar*)&lParam)[2];
			if (lParam & (1 << 24)) Ev.KeyCode = (Input::EKey)(Ev.KeyCode | 0x80);
			EventSrv->FireEvent(Ev);
			break;
		}

		//???how to handle situation when KeyDown was processed and Char should not be processed?
		case WM_CHAR:
		{
			WCHAR CharUTF16[2];
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&wParam, 1, CharUTF16, 1);

			Event::DisplayInput Ev;
			Ev.Type = Event::DisplayInput::CharInput;
			Ev.Char = CharUTF16[0];
			EventSrv->FireEvent(Ev);
			break;
		}

		case WM_INPUT:
		{
			RAWINPUT Data;
			PRAWINPUT pData = &Data;
			UINT DataSize = sizeof(Data);

			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pData, &DataSize, sizeof(RAWINPUTHEADER));

			if (Data.header.dwType == RIM_TYPEMOUSE && (Data.data.mouse.lLastX || Data.data.mouse.lLastY)) 
			{
				Event::DisplayInput Ev;
				Ev.Type = Event::DisplayInput::MouseMoveRaw;
				Ev.MouseInfo.x = Data.data.mouse.lLastX;
				Ev.MouseInfo.y = Data.data.mouse.lLastY;
				EventSrv->FireEvent(Ev);
			}

			DefRawInputProc(&pData, 1, sizeof(RAWINPUTHEADER));
			Result = 0;
			OK;
		}

		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			if (pParent) SetFocus(hWnd);

			Event::DisplayInput Ev;

			switch (uMsg)
			{
				case WM_LBUTTONDBLCLK:
				case WM_RBUTTONDBLCLK:
				case WM_MBUTTONDBLCLK:
					Ev.Type = Event::DisplayInput::MouseDblClick;
					break;

				case WM_LBUTTONDOWN:
				case WM_RBUTTONDOWN:
				case WM_MBUTTONDOWN:
					Ev.Type = Event::DisplayInput::MouseDown;
					SetCapture(hWnd);
					break;

				case WM_LBUTTONUP:
				case WM_RBUTTONUP:
				case WM_MBUTTONUP:
					Ev.Type = Event::DisplayInput::MouseUp;
					ReleaseCapture();
					break;
			}

			switch (uMsg)
			{
				case WM_LBUTTONDBLCLK:
				case WM_LBUTTONDOWN:
				case WM_LBUTTONUP:
					Ev.MouseInfo.Button = Input::MBLeft;
					break;

				case WM_RBUTTONDBLCLK:
				case WM_RBUTTONDOWN:
				case WM_RBUTTONUP:
					Ev.MouseInfo.Button = Input::MBRight;
					break;

				case WM_MBUTTONDBLCLK:
				case WM_MBUTTONDOWN:
				case WM_MBUTTONUP:
					Ev.MouseInfo.Button = Input::MBMiddle;
					break;
			}

			Ev.MouseInfo.x = GET_X_LPARAM(lParam);
			Ev.MouseInfo.y = GET_Y_LPARAM(lParam);
			EventSrv->FireEvent(Ev);
			break;
		}

		case WM_MOUSEMOVE:
		{
			Event::DisplayInput Ev;
			Ev.Type = Event::DisplayInput::MouseMove;
			Ev.MouseInfo.x = GET_X_LPARAM(lParam);
			Ev.MouseInfo.y = GET_Y_LPARAM(lParam);
			EventSrv->FireEvent(Ev);
			break;
		}

		case WM_MOUSEWHEEL:
		{
			Event::DisplayInput Ev;
			Ev.Type = Event::DisplayInput::MouseWheel;
			Ev.WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			EventSrv->FireEvent(Ev);
			break;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

}

#endif //__WIN32__

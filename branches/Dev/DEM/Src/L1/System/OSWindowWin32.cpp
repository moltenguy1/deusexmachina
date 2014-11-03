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
#define ACCEL_TOGGLEFULLSCREEN	1001

namespace Sys
{

COSWindowWin32::COSWindowWin32():
	WindowTitle("DeusExMachina - Untitled"),
	IsWndOpen(false),
	IsWndMinimized(false),
	AlwaysOnTop(false),
	hInst(NULL),
	hWnd(NULL),
	hWndParent(NULL),
	hAccel(NULL),
	aWndClass(0),
	StyleWindowed(WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE),
	StyleFullscreen(WS_POPUP | WS_SYSMENU | WS_VISIBLE),
	StyleChild(WS_CHILD | WS_TABSTOP | WS_VISIBLE) //???need tabstop?
{
	hInst = GetModuleHandle(NULL);
}
//---------------------------------------------------------------------

COSWindowWin32::~COSWindowWin32()
{
	if (IsWndOpen) CloseWindow();

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

bool COSWindowWin32::OpenWindow()
{
	n_assert(!IsWndOpen && hInst && !hWnd);

	// Send DisplayOpen event

	// Calculate adjusted window rect

	// Parent HWND handling
	if (hWndParent)
	{
		RECT r;
		GetClientRect(hWndParent, &r);
		//Width = (ushort)(r.right - r.left);
		//Height = (ushort)(r.bottom - r.top);
	}

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
		if (IconName.IsValid()) hIcon = LoadIcon(hInst, IconName.CStr());
		if (!hIcon) hIcon = LoadIcon(NULL, IDI_APPLICATION);

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
		aWndClass = RegisterClassEx(&WndClass);
		n_assert(aWndClass);
	}

	LONG WndStyle;
	if (hWndParent) WndStyle = StyleChild;
	else if (IsFullscreen) WndStyle = StyleFullscreen;
	else WndStyle = StyleWindowed;

	int X, Y, W, H;
	CalcWindowRect(X, Y, W, H);

	hWnd = CreateWindow((LPCSTR)(DWORD_PTR)aWndClass, WindowTitle.CStr(), WndStyle,
						X, Y, W, H,
						hWndParent, NULL, hInst, NULL);
	n_assert(hWnd);

	//!!!MSDN!
	//!!!The first time an application calls ShowWindow, it should use the WinMain function's
	//nCmdShow parameter as its nCmdShow parameter. Subsequent calls to ShowWindow must use
	//one of the values in the given list, instead of the one specified by the WinMain function's nCmdShow parameter. 

	if (AlwaysOnTop) SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

	SetWindowLongPtr(hWnd, 0, (LONG)this);

	RAWINPUTDEVICE RawInputDevices[1];
	RawInputDevices[0].usUsagePage = HID_USAGE_PAGE_GENERIC; 
	RawInputDevices[0].usUsage = HID_USAGE_GENERIC_MOUSE; 
	RawInputDevices[0].dwFlags = RIDEV_INPUTSINK;
	RawInputDevices[0].hwndTarget = hWnd;

	if (RegisterRawInputDevices(RawInputDevices, 1, sizeof(RAWINPUTDEVICE)) == FALSE)
		Sys::Log("COSWindowWin32: High-definition (raw) mouse device registration failed!\n");

	IsWndOpen = true;
	IsWndMinimized = false;
	OK;
}
//---------------------------------------------------------------------

void COSWindowWin32::CloseWindow()
{
	n_assert(IsWndOpen && hInst);

	// Close if not already closed externally by (e.g. by Alt-F4)
	if (hWnd)
	{
		DestroyWindow(hWnd);
		hWnd = NULL;
	}

	// send DisplayClose event

	IsWndOpen = false;
}
//---------------------------------------------------------------------

// Polls for and processes window messages. Call this message once per
// frame in your render loop. If the user clicks the window close
// button, or hits Alt-F4, an OnDisplayClose event will be sent.
void COSWindowWin32::ProcessWindowMessages()
{
	n_assert(IsWndOpen);

	// It may happen that the WinProc has already closed our window!
	if (!hWnd) return;

	// NB: we pass NULL instead of window handle to receive language switching messages
	MSG Msg;
	while (PeekMessage(&Msg, NULL /*hWnd*/, 0, 0, PM_REMOVE))
	{
		if (hAccel && TranslateAccelerator(hWnd, hAccel, &Msg) != FALSE) continue;
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
}
//---------------------------------------------------------------------

void COSWindowWin32::SetWindowTitle(const char* pTitle)
{
	WindowTitle = pTitle;
	if (hWnd) SetWindowText(hWnd, pTitle);
}
//---------------------------------------------------------------------

void COSWindowWin32::SetWindowIcon(const char* pIconName)
{
	IconName = pIconName;
	if (hWnd && IconName.IsValid())
	{
		HICON hIcon = LoadIcon(hInst, IconName.CStr());
		if (hIcon) SetClassLong(hWnd, GCL_HICON, (LONG)hIcon);
	}
}
//---------------------------------------------------------------------

void COSWindowWin32::CalcWindowRect(int& X, int& Y, int& W, int& H)
{
	if (hWndParent)
	{
		X = 0;
		Y = 0;

		RECT r;
		GetClientRect(hWndParent, &r);
		AdjustWindowRect(&r, StyleChild, FALSE);
		W = (ushort)(r.right - r.left); //???clamp w & h to parent rect?
		H = (ushort)(r.bottom - r.top);

		// Child window adjusts display mode values
		DisplayMode.PosX = X;
		DisplayMode.PosY = Y;
		DisplayMode.Width = W;
		DisplayMode.Height = H;
	}
	else
	{
		CMonitorInfo MonitorInfo;
		GetAdapterMonitorInfo(Adapter, MonitorInfo);

		if (IsFullscreen)
		{
			if (DisplayModeSwitchEnabled)
			{
				X = MonitorInfo.Left;
				Y = MonitorInfo.Top;
			}
			else
			{
				X = MonitorInfo.Left + ((MonitorInfo.Width - DisplayMode.Width) / 2);
				Y = MonitorInfo.Top + ((MonitorInfo.Height - DisplayMode.Height) / 2);
			}
			W = DisplayMode.Width;
			H = DisplayMode.Height;
		}
		else
		{
			X = MonitorInfo.Left + DisplayMode.PosX;
			Y = MonitorInfo.Top + DisplayMode.PosY;
			RECT r = { X, Y, X + DisplayMode.Width, Y + DisplayMode.Height };
			AdjustWindowRect(&r, StyleWindowed, FALSE);
			W = r.right - r.left;
			H = r.bottom - r.top;
		}
	}
}
//---------------------------------------------------------------------

void COSWindowWin32::ResetWindow()
{
	n_assert(hWnd && IsWndOpen);

	// IsFullscreen mode breaks theme (at least aero glass) on Win7, so restore it
	if (!hWndParent && !IsFullscreen) SetWindowTheme(hWnd, NULL, NULL);

	if (!hWndParent) SetWindowLongPtr(hWnd, GWL_STYLE, IsFullscreen ? StyleFullscreen : StyleWindowed);

	int X, Y, W, H;
	CalcWindowRect(X, Y, W, H);

	HWND hWndInsertAfter;
	if (AlwaysOnTop) hWndInsertAfter = HWND_TOPMOST;
	else if (hWndParent) hWndInsertAfter = hWndParent;
	else hWndInsertAfter = HWND_NOTOPMOST;

	SetWindowPos(hWnd, hWndInsertAfter, X, Y, W, H, SWP_NOACTIVATE | SWP_NOMOVE);
}
//---------------------------------------------------------------------

void COSWindowWin32::RestoreWindow()
{
	n_assert(hWnd && IsWndOpen);
	::ShowWindow(hWnd, SW_RESTORE);
	ResetWindow();
	IsWndMinimized = false;
}
//---------------------------------------------------------------------

void COSWindowWin32::MinimizeWindow()
{
	n_assert(hWnd && IsWndOpen);
	if (!IsWndMinimized)
	{
		if (!hWndParent) ShowWindow(hWnd, SW_MINIMIZE);
		IsWndMinimized = true;
	}
}
//---------------------------------------------------------------------

void COSWindowWin32::AdjustSize()
{
	n_assert(hWnd);

	RECT r;
	GetClientRect(hWnd, &r);
	DisplayMode.Width = (ushort)r.right;
	DisplayMode.Height = (ushort)r.bottom;

	EventSrv->FireEvent(CStrID("OnDisplaySizeChanged"));
}
//---------------------------------------------------------------------

LONG WINAPI COSWindowWin32::WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	COSWindowWin32* pDisp = (COSWindowWin32*)GetWindowLong(hWnd, 0);
	LONG Result = 0;
	if (pDisp && pDisp->HandleWindowMessage(hWnd, uMsg, wParam, lParam, Result)) return Result;
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------

//???need _hWnd param?
bool COSWindowWin32::HandleWindowMessage(HWND _hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LONG Result)
{
	switch (uMsg)
	{
		case WM_SYSCOMMAND:
			// Prevent moving/sizing and power loss in fullscreen mode
			if (IsFullscreen)
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

		case WM_ERASEBKGND:
			// Prevent Windows from erasing the background
			Result = 1;
			OK;

		case WM_SIZE:
			if (wParam == SIZE_MAXHIDE || wParam == SIZE_MINIMIZED)
			{
				if (!IsWndMinimized)
				{
					IsWndMinimized = true;
					EventSrv->FireEvent(CStrID("OnDisplayMinimized"));
					ReleaseCapture();
				}
			}
			else
			{
				if (IsWndMinimized)
				{
					IsWndMinimized = false;
					EventSrv->FireEvent(CStrID("OnDisplayRestored"));
					ReleaseCapture();
				}
				if (hWnd && AutoAdjustSize && !IsFullscreen) AdjustSize();
			}
			// Manually change window size in child mode
			if (hWndParent) MoveWindow(hWnd, DisplayMode.PosX, DisplayMode.PosY, LOWORD(lParam), HIWORD(lParam), TRUE);
			break;

		case WM_SETCURSOR:
			if (EventSrv->FireEvent(CStrID("OnDisplaySetCursor")))
			{
				Result = TRUE;
				OK;
			}
			break;

		case WM_PAINT:
			EventSrv->FireEvent(CStrID("OnDisplayPaint"));
			break;

		case WM_SETFOCUS:
			EventSrv->FireEvent(CStrID("OnDisplaySetFocus"));
			ReleaseCapture();
			break;

		case WM_KILLFOCUS:
			EventSrv->FireEvent(CStrID("OnDisplayKillFocus"));
			ReleaseCapture();
			break;

		case WM_CLOSE:
			EventSrv->FireEvent(CStrID("OnDisplayClose"));
			hWnd = NULL;
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ACCEL_TOGGLEFULLSCREEN:
					EventSrv->FireEvent(CStrID("OnDisplayToggleFullscreen"));
					break;
			}
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
			if (hWndParent) SetFocus(hWnd);

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
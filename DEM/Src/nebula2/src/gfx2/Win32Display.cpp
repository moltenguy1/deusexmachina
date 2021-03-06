#ifdef __WIN32__
#include "gfx2/Win32Display.h"

#include <Events/EventManager.h>
#include <Gfx/Events/DisplayInput.h>
#include "kernel/nkernelserver.h"

//???in what header?
#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC	((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE	((USHORT) 0x02)
#endif

CWin32Display::CWin32Display():
	WindowTitle("Nebula 2 - Untitled"),
	IsWndOpen(false),
	Fullscreen(false),
	AlwaysOnTop(false),
	AutoAdjustSize(true),
	hInst(NULL),
	hWnd(NULL),
	hWndParent(NULL),
	hAccel(NULL),
	aWndClass(0),
	StyleWindowed(WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE),
	StyleFullscreen(WS_POPUP | WS_SYSMENU | WS_VISIBLE),
	StyleChild(WS_CHILD | WS_VISIBLE) //WS_CHILD | WS_TABSTOP - N2, now N3 variant is selected
{
	hInst = GetModuleHandle(NULL);
}
//---------------------------------------------------------------------

CWin32Display::~CWin32Display()
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
			n_error("CWin32Display::CloseWindow(): UnregisterClass() failed!\n");
		aWndClass = 0;
	}
}
//---------------------------------------------------------------------

// Open the application window. Override this method in a platform specific subclass. The window
// will live during the entire life span of the graphics server. It should be opened in a minimized
// or invisible state, until nGfxServer2::OpenDisplay() calls AdjustWindowForChange() and RestoreWindow().
bool CWin32Display::OpenWindow()
{
	n_assert(!IsWndOpen && hInst && !hWnd);

	// Send DisplayOpen event

	// Calculate adjusted window rect

	//!!!in N3 this is done through SetParentWindow!
	// Parent HWND handling
	if (CoreSrv->GetGlobal<int>("parent_hwnd", (int&)hWndParent))
	{
		RECT r;
		GetClientRect(hWndParent, &r);
		DisplayMode.Width = (ushort)(r.right - r.left);
		DisplayMode.Height = (ushort)(r.bottom - r.top);
	}
	else hWndParent = NULL;

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
		if (IconName.IsValid()) hIcon = LoadIcon(hInst, IconName.Get());
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
		WndClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		WndClass.lpszMenuName  = NULL;
		WndClass.lpszClassName = NEBULA2_WINDOW_CLASS;
		WndClass.hIconSm       = NULL;
		aWndClass = RegisterClassEx(&WndClass);
		n_assert(aWndClass);
	}

	DWORD WndStyle;
	if (hWndParent) WndStyle = StyleChild;
	else if (Fullscreen) WndStyle = StyleFullscreen;
	else WndStyle = StyleWindowed;

	int W, H;
	CalcWindowRect(W, H);

	hWnd = CreateWindow((LPCSTR)aWndClass, WindowTitle.Get(), WndStyle,
						DisplayMode.PosX, DisplayMode.PosY, W, H,
						hWndParent, NULL, hInst, NULL);
	n_assert(hWnd);

	if (AlwaysOnTop) SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

	SetWindowLong(hWnd, 0, (LONG)this);

	CoreSrv->SetGlobal("hwnd", (int)hWnd);

	RAWINPUTDEVICE RavInputDevices[1];
	RavInputDevices[0].usUsagePage = HID_USAGE_PAGE_GENERIC; 
	RavInputDevices[0].usUsage = HID_USAGE_GENERIC_MOUSE; 
	RavInputDevices[0].dwFlags = RIDEV_INPUTSINK;
	RavInputDevices[0].hwndTarget = hWnd;

	if (RegisterRawInputDevices(RavInputDevices, 1, sizeof(RAWINPUTDEVICE)) == FALSE)
		n_printf("CWin32Display: High-definition (raw) mouse device registration failed!\n");

	IsWndOpen = true;
	IsWndMinimized = false;
	return true;
}
//---------------------------------------------------------------------

// Close the application window. This method will be called from the gfx server's destructor.
// Override this method in a platform specific subclass.
void CWin32Display::CloseWindow()
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
// button, or hits Alt-F4, a CloseRequested input event will be sent.
void CWin32Display::ProcessWindowMessages()
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

void CWin32Display::SetWindowTitle(const char* pTitle)
{
	WindowTitle = pTitle;
	if (hWnd) SetWindowText(hWnd, pTitle);
}
//---------------------------------------------------------------------

void CWin32Display::SetWindowIcon(const char* pIconName)
{
	IconName = pIconName;
	if (hWnd && IconName.IsValid())
	{
		HICON hIcon = LoadIcon(hInst, IconName.Get());
		if (hIcon) SetClassLong(hWnd, GCL_HICON, (LONG)hIcon);
	}
}
//---------------------------------------------------------------------

void CWin32Display::CalcWindowRect(int& W, int& H)
{
	if (hWndParent)
	{
		RECT r;
		GetClientRect(hWndParent, &r);
		AdjustWindowRect(&r, StyleChild, 0);
		DisplayMode.PosX = 0;
		DisplayMode.PosY = 0;
		DisplayMode.Width = (ushort)(r.right - r.left); //???clamp w & h to parent rect?
		DisplayMode.Height = (ushort)(r.bottom - r.top);
		W = DisplayMode.Width;
		H = DisplayMode.Height;
	}
	else if (Fullscreen)
	{
		W = DisplayMode.Width;
		H = DisplayMode.Height;
	}
	else
	{
		RECT r = {0, 0, DisplayMode.Width, DisplayMode.Height};
		AdjustWindowRect(&r, StyleWindowed, 0);
		W = r.right - r.left;
		H = r.bottom - r.top;
	}
}
//---------------------------------------------------------------------

void CWin32Display::RestoreWindow()
{
	n_assert(hWnd && IsWndOpen);

	ShowWindow(hWnd, SW_RESTORE);

	int W, H;
	CalcWindowRect(W, H);

	HWND hWndPlacement;
	if (AlwaysOnTop) hWndPlacement = HWND_TOPMOST;
	else if (hWndParent) hWndPlacement = hWndParent;
	else hWndPlacement = HWND_NOTOPMOST;

	SetWindowPos(hWnd, hWndPlacement, DisplayMode.PosX, DisplayMode.PosY, W, H, SWP_SHOWWINDOW);

	//!!!IF WM_SIZE IS CALLED, REMOVE STRING BELOW!  (see WM_SIZE)
	// Manually change window size in child mode
	if (hWndParent) MoveWindow(hWnd, DisplayMode.PosX, DisplayMode.PosY, W, H, TRUE);
	IsWndMinimized = false;
}
//---------------------------------------------------------------------

void CWin32Display::MinimizeWindow()
{
	n_assert(hWnd && IsWndOpen);
	if (!IsWndMinimized)
	{
		if (!hWndParent) ShowWindow(hWnd, SW_MINIMIZE);
		IsWndMinimized = true;
	}
}
//---------------------------------------------------------------------

LONG WINAPI CWin32Display::WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CWin32Display* pDisp = (CWin32Display*)GetWindowLong(hWnd, 0);
	LONG Result = 0;
	if (pDisp->HandleWindowMessage(hWnd, uMsg, wParam, lParam, Result)) return Result;
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------

//???need _hWnd param?
bool CWin32Display::HandleWindowMessage(HWND _hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LONG Result)
{
	switch (uMsg)
	{
		case WM_SYSCOMMAND:
			// Prevent moving/sizing and power loss in fullscreen mode
			if (Fullscreen)
			{
				switch (wParam)
				{
					case SC_MOVE:
					case SC_SIZE:
					case SC_MAXIMIZE:
					case SC_KEYMENU:
					case SC_MONITORPOWER:
						Result = 1;
						return true;
				}
			}
			break;

		case WM_ERASEBKGND:
			// Prevent Windows from erasing the background
			Result = 1;
			return true;

		case WM_SIZE:
			if (wParam == SIZE_MAXHIDE || wParam == SIZE_MINIMIZED)
			{
				IsWndMinimized = true;
				EventMgr->FireEvent(CStrID("OnDisplayMinimized"));
				ReleaseCapture();
			}
			else
			{
				IsWndMinimized = false;
				EventMgr->FireEvent(CStrID("OnDisplayRestored"));
				// As a child window, do not release capture, because it would block the resizing
				if (!hWndParent) ReleaseCapture();
				if (hWnd && AutoAdjustSize) AdjustSize();
			}
			// Manually change window size in child mode
			//!!!x & y were 0
			if (hWndParent) MoveWindow(hWnd, DisplayMode.PosX, DisplayMode.PosY, LOWORD(lParam), HIWORD(lParam), TRUE);
			break;

		case WM_SETCURSOR:
			if (EventMgr->FireEvent(CStrID("OnDisplaySetCursor")))
			{
				Result = TRUE;
				return true;
			}
			break;

		case WM_PAINT:
			EventMgr->FireEvent(CStrID("OnDisplayPaint"));
			break;

		case WM_SETFOCUS:
			EventMgr->FireEvent(CStrID("OnDisplaySetFocus"));
			ReleaseCapture();
			break;

		case WM_KILLFOCUS:
			EventMgr->FireEvent(CStrID("OnDisplayKillFocus"));
			ReleaseCapture();
			break;

		case WM_CLOSE:
			EventMgr->FireEvent(CStrID("OnDisplayClose"));
			hWnd = NULL;
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ACCEL_TOGGLEFULLSCREEN:
					EventMgr->FireEvent(CStrID("OnDisplayToggleFullscreen"));
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
			EventMgr->FireEvent(Ev);
			break;
		}

		case WM_CHAR:
		{
			WCHAR CharUTF16[2];
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&wParam, 1, CharUTF16, 1);

			Event::DisplayInput Ev;
			Ev.Type = Event::DisplayInput::CharInput;
			Ev.Char = CharUTF16[0];
			EventMgr->FireEvent(Ev);
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
				EventMgr->FireEvent(Ev);
			}

			DefRawInputProc(&pData, 1, sizeof(RAWINPUTHEADER));
			Result = 0;
			return true;
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

			Ev.MouseInfo.x = LOWORD(lParam);
			Ev.MouseInfo.y = HIWORD(lParam);
			EventMgr->FireEvent(Ev);
			break;
		}

		case WM_MOUSEMOVE:
		{
			Event::DisplayInput Ev;
			Ev.Type = Event::DisplayInput::MouseMove;
			Ev.MouseInfo.x = LOWORD(lParam);
			Ev.MouseInfo.y = HIWORD(lParam);
			EventMgr->FireEvent(Ev);
			break;
		}

		case WM_MOUSEWHEEL:
		{
			Event::DisplayInput Ev;
			Ev.Type = Event::DisplayInput::MouseWheel;
			Ev.WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			EventMgr->FireEvent(Ev);
			break;
		}
	}

	return false;
}
//---------------------------------------------------------------------

//???change here & listen event in D3D9 handler, compare D3D9 settings with curr display mode & reset device if needed>?
void CWin32Display::AdjustSize()
{
	n_assert(hWnd);
	RECT r;
	GetClientRect(hWnd, &r);
	DisplayMode.Width = (ushort)r.right;
	DisplayMode.Height = (ushort)r.bottom;
}
//---------------------------------------------------------------------

//void D3D9DisplayDevice::AdjustSize()
//{
//	CDisplayMode oldDisplayMode = DisplayMode;
//	Win32DisplayDevice::AdjustSize();
//	if (oldDisplayMode != DisplayMode && D3D9RenderDevice::Instance()->IsOpen()) D3D9RenderDevice::Instance()->ResetDevice();
//}
////---------------------------------------------------------------------
//DisplayMode
//D3D9DisplayDevice::ComputeAdjustedWindowRect()
//{
//    if (0 != this->parentWindow)
//    {
//        // if we're a child window, let parent class handle it
//        return Win32DisplayDevice::ComputeAdjustedWindowRect();
//    }
//    else
//    {
//        // get monitor handle of selected adapter
//        IDirect3D9* pD3D9 = D3D9RenderDevice::GetDirect3D();
//        n_assert(this->AdapterExists(this->adapter));
//        HMONITOR hMonitor = pD3D9->GetAdapterMonitor(this->adapter);
//        MONITORINFO monitorInfo = { sizeof(monitorInfo), 0 };
//        GetMonitorInfo(hMonitor, &monitorInfo);
//        int monitorOriginX = monitorInfo.rcMonitor.left;
//        int monitorOriginY = monitorInfo.rcMonitor.top;
//        
//        if (this->fullscreen)
//        {
//            // running in fullscreen mode
//            if (!this->IsDisplayModeSwitchEnabled())
//            {
//                // running in "fake" fullscreen mode center the window on the desktop
//                int monitorWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
//                int monitorHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
//                int x = monitorOriginX + ((monitorWidth - this->displayMode.GetWidth()) / 2);
//                int y = monitorOriginY + ((monitorHeight - this->displayMode.GetHeight()) / 2);
//                return DisplayMode(x, y, this->displayMode.GetWidth(), this->displayMode.GetHeight(), this->displayMode.GetPixelFormat());
//            }
//            else
//            {
//                // running in normal fullscreen mode
//                return DisplayMode(monitorOriginX, monitorOriginY, this->displayMode.GetWidth(), this->displayMode.GetHeight(), this->displayMode.GetPixelFormat());
//            }
//        }
//        else
//        {
//            // need to make sure that the window client area is the requested width
//            int adjXPos = monitorOriginX + displayMode.GetXPos();
//            int adjYPos = monitorOriginY + displayMode.GetYPos();
//            RECT adjRect;
//            adjRect.left   = adjXPos;
//            adjRect.right  = adjXPos + displayMode.GetWidth();
//            adjRect.top    = adjYPos;
//            adjRect.bottom = adjYPos + displayMode.GetHeight();
//            AdjustWindowRect(&adjRect, this->windowedStyle, 0);
//            int adjWidth = adjRect.right - adjRect.left;
//            int adjHeight = adjRect.bottom - adjRect.top;
//            return DisplayMode(adjXPos, adjYPos, adjWidth, adjHeight, displayMode.GetPixelFormat());
//        }
//    }
//}

#endif __WIN32__

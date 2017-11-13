#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <windows.h>

#pragma comment(lib, "USER32")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

#define KEYDOWN(k) ((k) & 0x80)

// This is a **very** minimal hotcorner app, written in C. Maybe its not the
// optimal way to do this, but it works for me.
//
// Zero state is stored anywhere, no registry keys or configuration files.
//
// - If you want to configure something, edit the code.
// - If you want to uninstall it, just delete it.
//
// Tavis Ormandy <taviso@cmpxchg8b.com> December, 2016
//
// https://github.com/taviso/hotcorner
//

// If the mouse enters this rectangle, activate the hot corner function.
// There are some hints about changing corners here
//      https://github.com/taviso/hotcorner/issues/7#issuecomment-269367351
static const int radius = 20;
static const int padding = 1000;

static RECT kHotCornerLT;
static RECT kHotCornerRT;
static RECT kHotCornerLB;
static RECT kHotCornerRB;

// Initialize the corner rectangles
static DWORD initCorners() {
	// Left Top
	kHotCornerLT.top = -padding;
	kHotCornerLT.left = -padding;
	kHotCornerLT.right = radius;
	kHotCornerLT.bottom = radius;
	// Right Top
	kHotCornerRT.top = -padding;
	kHotCornerRT.left = GetSystemMetrics(SM_CXSCREEN) - radius;
	kHotCornerRT.right = GetSystemMetrics(SM_CXSCREEN) + padding;
    kHotCornerRT.bottom = radius;
	// Left Bottom
	kHotCornerLB.top = GetSystemMetrics(SM_CYSCREEN) - radius;
	kHotCornerLB.left = -padding;
	kHotCornerLB.right = radius;
	kHotCornerLB.bottom = GetSystemMetrics(SM_CYSCREEN) + padding;
	// Right Bottom
	kHotCornerRB.top = GetSystemMetrics(SM_CYSCREEN) - radius;
	kHotCornerRB.left = GetSystemMetrics(SM_CXSCREEN) - radius;
	kHotCornerRB.right = GetSystemMetrics(SM_CXSCREEN) + padding;
	kHotCornerRB.bottom = GetSystemMetrics(SM_CYSCREEN) + padding;
	return 0;
}

// Input to inject when left top corner activated (Win)
static const INPUT kCornerInputLT[] = {
    { INPUT_KEYBOARD, .ki = { VK_LWIN, .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_LWIN, .dwFlags = KEYEVENTF_KEYUP }},
};
// Input to inject when right top corner activated (Win+Up)
static const INPUT kCornerInputRT[] = {
    { INPUT_KEYBOARD, .ki = { VK_LWIN, .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_UP,  .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_UP,  .dwFlags = KEYEVENTF_KEYUP }},
    { INPUT_KEYBOARD, .ki = { VK_LWIN, .dwFlags = KEYEVENTF_KEYUP }},
};
// Input to inject when right top corner activated (Win+Down)
static const INPUT kCornerInputRT2[] = {
	{ INPUT_KEYBOARD,.ki = { VK_LWIN,.dwFlags = 0 } },
	{ INPUT_KEYBOARD,.ki = { VK_DOWN,.dwFlags = 0 } },
	{ INPUT_KEYBOARD,.ki = { VK_DOWN,.dwFlags = KEYEVENTF_KEYUP } },
	{ INPUT_KEYBOARD,.ki = { VK_LWIN,.dwFlags = KEYEVENTF_KEYUP } },
};
// Input to inject when left bottom corner activated (Win+Tab)
static const INPUT kCornerInputLB[] = {
    { INPUT_KEYBOARD, .ki = { VK_LWIN, .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_TAB,  .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { VK_TAB,  .dwFlags = KEYEVENTF_KEYUP }},
    { INPUT_KEYBOARD, .ki = { VK_LWIN, .dwFlags = KEYEVENTF_KEYUP }},
};
// Input to inject when right bottom corner activated (Win+Tab)
static const INPUT kCornerInputRB[] = {
    { INPUT_KEYBOARD, .ki = { VK_LWIN, .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { 'M',  .dwFlags = 0 }},
    { INPUT_KEYBOARD, .ki = { 'M',  .dwFlags = KEYEVENTF_KEYUP }},
    { INPUT_KEYBOARD, .ki = { VK_LWIN, .dwFlags = KEYEVENTF_KEYUP }},
};

// How long cursor has to linger in the kHotCornerLT RECT to trigger input.
static const DWORD kHotDelay = 100;

// You can exit the application using the hot key CTRL+ALT+C by default, if it
// interferes with some application you're using (e.g. a full screen game).
static const DWORD kHotKeyModifiers = MOD_CONTROL | MOD_ALT;
static const DWORD kHotKey = 'C';

static HANDLE CornerThread = INVALID_HANDLE_VALUE;

// This thread runs when the cursor enters the hot corner, and waits to see if the cursor stays in the corner.
// If the mouse leaves while we're waiting, the thread is just terminated.
static DWORD WINAPI CornerHotFunc(LPVOID lpParameter)
{
    BYTE KeyState[256];
    POINT Point;
	HWND foregroundWindow = GetForegroundWindow();

    Sleep(kHotDelay);

    // Check if a mouse putton is pressed, maybe a drag operation?
    if (GetKeyState(VK_LBUTTON) < 0 || GetKeyState(VK_RBUTTON) < 0) {
        return 0;
    }

    // Check if any modifier keys are pressed.
    if (GetKeyboardState(KeyState)) {
        if (KEYDOWN(KeyState[VK_SHIFT]) || KEYDOWN(KeyState[VK_CONTROL])
          || KEYDOWN(KeyState[VK_MENU]) || KEYDOWN(KeyState[VK_LWIN])
          || KEYDOWN(KeyState[VK_RWIN])) {
            return 0;
        }
    }

    // Verify the corner is still hot
    if (GetCursorPos(&Point) == FALSE) {
        return 1;
    }

    // Check co-ordinates.
    if (PtInRect(&kHotCornerLT, Point)) {
        #pragma warning(suppress : 4090)
        if (SendInput(_countof(kCornerInputLT), kCornerInputLT, sizeof(INPUT)) != _countof(kCornerInputLT)) {
            return 1;
        }
	}
	else if (PtInRect(&kHotCornerRT, Point)) {
		// If the foreground window is maximized
		if (foregroundWindow && !IsZoomed(foregroundWindow))
		{
			// Win + Up
			#pragma warning(suppress : 4090)
			if (SendInput(_countof(kCornerInputRT), kCornerInputRT, sizeof(INPUT)) != _countof(kCornerInputRT)) {
				return 1;
			}
		}
		else {
			// Win + Down
			#pragma warning(suppress : 4090)
			if (SendInput(_countof(kCornerInputRT2), kCornerInputRT2, sizeof(INPUT)) != _countof(kCornerInputRT2)) {
				return 1;
			}
		}
	}
	else if (PtInRect(&kHotCornerLB, Point)) {
		#pragma warning(suppress : 4090)
		if (SendInput(_countof(kCornerInputLB), kCornerInputLB, sizeof(INPUT)) != _countof(kCornerInputLB)) {
			return 1;
		}
	}
	else if (PtInRect(&kHotCornerRB, Point)) {
		#pragma warning(suppress : 4090)
		// If the foreground is minimized
		if (foregroundWindow) {
			// Win + M
			#pragma warning(suppress : 4090)
			if (SendInput(_countof(kCornerInputRB), kCornerInputRB, sizeof(INPUT)) != _countof(kCornerInputRB)) {
				return 1;
			}
		}
	}

    return 0;
}

static LRESULT CALLBACK MouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
    MSLLHOOKSTRUCT *evt = (MSLLHOOKSTRUCT *) lParam;

    // If the mouse hasn't moved, we're done.
    if (wParam != WM_MOUSEMOVE)
        goto finish;

    // Check if the cursor is hot or cold.
    if (!PtInRect(&kHotCornerLT, evt->pt)
		&& !PtInRect(&kHotCornerRT, evt->pt)
		&& !PtInRect(&kHotCornerLB, evt->pt)
        && !PtInRect(&kHotCornerRB, evt->pt)
    ) {

        // The corner is cold, and was cold before.
        if (CornerThread == INVALID_HANDLE_VALUE)
            goto finish;

        // The corner is cold, but was previously hot.
        TerminateThread(CornerThread, 0);

        CloseHandle(CornerThread);

        // Reset state.
        CornerThread = INVALID_HANDLE_VALUE;

        goto finish;
    }

    // The corner is hot, check if it was already hot.
    if (CornerThread != INVALID_HANDLE_VALUE) {
        goto finish;
    }

    // Check if a mouse putton is pressed, maybe a drag operation?
    if (GetKeyState(VK_LBUTTON) < 0 || GetKeyState(VK_RBUTTON) < 0) {
        goto finish;
    }

    // The corner is hot, and was previously cold. Here we start a thread to
    // monitor if the mouse lingers.
    CornerThread = CreateThread(NULL, 0, CornerHotFunc, NULL, 0, NULL);

finish:
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG Msg;
    HHOOK MouseHook;

    if (!(MouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookCallback, NULL, 0)))
        return 1;

    initCorners();

    RegisterHotKey(NULL, 1, kHotKeyModifiers, kHotKey);

    while (GetMessage(&Msg, NULL, 0, 0)) {
        if (Msg.message == WM_HOTKEY) {
            break;
        }
        DispatchMessage(&Msg);
    }

    UnhookWindowsHookEx(MouseHook);

    return Msg.wParam;
}

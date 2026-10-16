
#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_OPEN 1001
#define ID_TRAY_EXIT 1002
#define ID_TRAYICON 1

NOTIFYICONDATAW nid = {};
HWND g_hwnd = nullptr;

void ShowMainWindow()
{
    ShowWindow(g_hwnd, SW_SHOW);
    ShowWindow(g_hwnd, SW_RESTORE);
    SetForegroundWindow(g_hwnd);
}

void ShowTrayMenu()
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    if (!menu)
        return;

    AppendMenuW(menu, MF_STRING, ID_TRAY_OPEN, L"Открыть");
    AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"Выход");

    SetForegroundWindow(g_hwnd);

    TrackPopupMenu(
        menu,
        TPM_RIGHTBUTTON | TPM_BOTTOMALIGN,
        pt.x, pt.y, 0, g_hwnd, nullptr
    );

    DestroyMenu(menu);
}

LRESULT CALLBACK WndProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        nid.cbSize = sizeof(nid);
        nid.hWnd = hwnd;
        nid.uID = ID_TRAYICON;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        lstrcpynW(nid.szTip, L"TrayApp", ARRAYSIZE(nid.szTip));

        if (!Shell_NotifyIconW(NIM_ADD, &nid))
        {
            MessageBoxW(hwnd, L"Не удалось добавить значок в трей.",
                L"TrayApp", MB_OK | MB_ICONERROR);
            return -1;
        }
        break;

    case WM_TRAYICON:
        if (lParam == WM_LBUTTONUP)
            ShowMainWindow();
        else if (lParam == WM_RBUTTONUP)
            ShowTrayMenu();
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_TRAY_OPEN:
            ShowMainWindow();
            break;

        case ID_TRAY_EXIT:
            DestroyWindow(hwnd);
            break;
        }
        break;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        Shell_NotifyIconW(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    return 0;
}

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"TrayAppWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    if (!RegisterClassW(&wc))
        return 1;

    g_hwnd = CreateWindowExW(
        0, CLASS_NAME, L"TrayApp",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!g_hwnd)
        return 1;

    ShowWindow(g_hwnd, nCmdShow);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}


#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON (WM_USER + 1)

#define ID_TRAY_OPEN  1001
#define ID_TRAY_EXIT  1002
#define ID_FILE_EXIT  1003
#define ID_TRAYICON   1

// Имя mutex уникально для приложения в текущем сеансе Windows.
constexpr wchar_t MUTEX_NAME[] = L"Local\\TrayApp_Shinigami3712";

NOTIFYICONDATAW nid = {};
HWND g_hwnd = nullptr;
HANDLE g_mutex = nullptr;
UINT g_taskbarCreated = 0;
bool g_hiddenStart = false;

void ShowMainWindow()
{
    if (!g_hwnd)
        return;

    ShowWindow(g_hwnd, SW_SHOW);
    ShowWindow(g_hwnd, SW_RESTORE);
    SetForegroundWindow(g_hwnd);
}

bool AddTrayIcon()
{
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_hwnd;
    nid.uID = ID_TRAYICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    lstrcpynW(nid.szTip, L"TrayApp", ARRAYSIZE(nid.szTip));

    return Shell_NotifyIconW(NIM_ADD, &nid) != FALSE;
}

void ShowTrayMenu()
{
    POINT pt{};
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

    // Нужно для корректного закрытия меню в некоторых сценариях.
    PostMessageW(g_hwnd, WM_NULL, 0, 0);

    DestroyMenu(menu);
}

void CreateMainMenu(HWND hwnd)
{
    HMENU menuBar = CreateMenu();
    HMENU fileMenu = CreatePopupMenu();

    if (!menuBar || !fileMenu)
    {
        if (menuBar)
            DestroyMenu(menuBar);

        if (fileMenu)
            DestroyMenu(fileMenu);

        return;
    }

    AppendMenuW(fileMenu, MF_STRING, ID_FILE_EXIT, L"Выход");
    AppendMenuW(menuBar, MF_POPUP,
        reinterpret_cast<UINT_PTR>(fileMenu), L"Файл");

    SetMenu(hwnd, menuBar);
}

LRESULT CALLBACK WndProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Explorer может перезапускаться, например после сбоя.
    if (msg == g_taskbarCreated)
    {
        AddTrayIcon();
        return 0;
    }

    switch (msg)
    {
    case WM_CREATE:
        if (!AddTrayIcon())
        {
            MessageBoxW(
                hwnd,
                L"Не удалось добавить значок в трей.",
                L"TrayApp",
                MB_OK | MB_ICONERROR
            );

            return -1;
        }

        CreateMainMenu(hwnd);
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
        case ID_FILE_EXIT:
            DestroyWindow(hwnd);
            break;
        }
        break;

    case WM_CLOSE:
        // Закрытие окна не завершает приложение.
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
    PWSTR lpCmdLine,
    int nCmdShow)
{
    // Поддерживается запуск: TrayApp.exe --hidden
    if (lpCmdLine &&
        (wcsstr(lpCmdLine, L"--hidden") != nullptr ||
            wcsstr(lpCmdLine, L"/hidden") != nullptr))
    {
        g_hiddenStart = true;
    }

    // Проверяем наличие другого экземпляра до создания окна
    // и до добавления значка в системный трей.
    g_mutex = CreateMutexW(nullptr, FALSE, MUTEX_NAME);

    if (!g_mutex)
        return 1;

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(g_mutex);
        g_mutex = nullptr;
        return 0;
    }

    // Сообщение приходит окну после пересоздания панели задач.
    g_taskbarCreated =
        RegisterWindowMessageW(L"TaskbarCreated");

    if (!g_taskbarCreated)
    {
        CloseHandle(g_mutex);
        return 1;
    }

    const wchar_t CLASS_NAME[] = L"TrayAppWindow";

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    if (!RegisterClassW(&wc))
    {
        CloseHandle(g_mutex);
        return 1;
    }

    g_hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"TrayApp",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        600, 400,
        nullptr, nullptr,
        hInstance, nullptr
    );

    if (!g_hwnd)
    {
        CloseHandle(g_mutex);
        return 1;
    }

    // При скрытом запуске окно не показывается,
    // но приложение и значок в трее продолжают работать.
    if (!g_hiddenStart)
        ShowWindow(g_hwnd, nCmdShow);

    MSG msg{};

    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_mutex)
    {
        CloseHandle(g_mutex);
        g_mutex = nullptr;
    }

    return static_cast<int>(msg.wParam);
}

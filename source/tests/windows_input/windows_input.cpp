/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include "stdafx.h"
#include "shared/logger.h"
#include "input.h"

constexpr WCHAR CLASS_NAME[] = L"windows_input";
constexpr WCHAR WINDOW_NAME[] = L"windows_input";

ATOM             MyRegisterClass(HINSTANCE);
HWND             InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MyRegisterClass(hInstance);

    // Perform application initialization:
    HWND hWnd = InitInstance(hInstance, nCmdShow);
    if (!hWnd)
    {
        return FALSE;
    }

    MSG msg;

    // Main message loop:
    bool running = true;
    while (running)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Input::PreRender();

        /*
         * Force a Hourglass frame boundary.
         */
        HDC hdc = GetDC(hWnd);
        SwapBuffers(hdc);
        ReleaseDC(hWnd, hdc);

        Input::PostRender();
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};

    wcex.cbSize        = sizeof(wcex);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.hInstance     = hInstance;
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszClassName = CLASS_NAME;

    return RegisterClassExW(&wcex);
}

HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd = CreateWindowW(CLASS_NAME,
                             WINDOW_NAME,
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT,
                             0,
                             CW_USEDEFAULT,
                             0,
                             nullptr,
                             nullptr,
                             hInstance,
                             nullptr);

   if (!hWnd)
   {
      return nullptr;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return hWnd;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#define HANDLE_KEYBOARD_MESSAGE(msg)                                      \
    case msg:                                                             \
    {                                                                     \
        std::wostringstream oss;                                          \
        oss.setf(std::ios_base::showbase);                                \
        oss.setf(std::ios_base::hex, std::ios_base::basefield);           \
                                                                          \
        oss << #msg L": wParam = " << wParam \
            << L"; lParam = " << std::bitset<sizeof(lParam) * 8>(lParam); \
                                                                          \
        Logger::WriteLine(oss.str());                                     \
        Input::LogData();                                                 \
    }                                                                     \
    break

#define HANDLE_MOUSE_MESSAGE(msg)                                                            \
    case msg:                                                                                \
    {                                                                                        \
        std::wostringstream oss;                                                             \
                                                                                             \
        oss << #msg L": wParam = " << std::bitset<sizeof(wParam) * 8>(wParam)                \
            << L"; xPos = " << GET_X_LPARAM(lParam) << L"; yPos = " << GET_Y_LPARAM(lParam); \
                                                                                             \
        Logger::WriteLine(oss.str());                                                        \
        Input::LogData();                                                                    \
    }                                                                                        \
    break

    switch (message)
    {
    HANDLE_KEYBOARD_MESSAGE(WM_KEYDOWN);
    HANDLE_KEYBOARD_MESSAGE(WM_KEYUP);
    HANDLE_KEYBOARD_MESSAGE(WM_SYSKEYDOWN);
    HANDLE_KEYBOARD_MESSAGE(WM_SYSKEYUP);

    HANDLE_MOUSE_MESSAGE(WM_MOUSEMOVE);
    HANDLE_MOUSE_MESSAGE(WM_LBUTTONDOWN);
    HANDLE_MOUSE_MESSAGE(WM_LBUTTONUP);
    HANDLE_MOUSE_MESSAGE(WM_LBUTTONDBLCLK);
    HANDLE_MOUSE_MESSAGE(WM_RBUTTONDOWN);
    HANDLE_MOUSE_MESSAGE(WM_RBUTTONUP);
    HANDLE_MOUSE_MESSAGE(WM_RBUTTONDBLCLK);
    HANDLE_MOUSE_MESSAGE(WM_MBUTTONDOWN);
    HANDLE_MOUSE_MESSAGE(WM_MBUTTONUP);
    HANDLE_MOUSE_MESSAGE(WM_MBUTTONDBLCLK);
    HANDLE_MOUSE_MESSAGE(WM_XBUTTONDOWN);
    HANDLE_MOUSE_MESSAGE(WM_XBUTTONUP);
    HANDLE_MOUSE_MESSAGE(WM_XBUTTONDBLCLK);

    case WM_MOUSEWHEEL:
    {
        std::wostringstream oss;

        const auto keystate = GET_KEYSTATE_WPARAM(wParam);
        const auto delta = GET_WHEEL_DELTA_WPARAM(wParam);

        oss << L"WM_MOUSEWHEEL: keystate = " << std::bitset<sizeof(keystate) * 8>(keystate)
            << L"; delta = " << delta
            << L"; xPos = " << GET_X_LPARAM(lParam) << L"; yPos = " << GET_Y_LPARAM(lParam);

        Logger::WriteLine(oss.str());
        Input::LogData();
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

#undef HANDLE_KEYBOARD_MESSAGE
#undef HANDLE_MOUSE_MESSAGE

    return 0;
}

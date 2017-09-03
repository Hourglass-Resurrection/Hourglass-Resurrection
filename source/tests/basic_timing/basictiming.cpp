/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include "stdafx.h"
#include "shared/timing.h"
#include "time_stats.h"

constexpr WCHAR CLASS_NAME[] = L"basic_timing";
constexpr WCHAR WINDOW_NAME[] = L"basic_timing";

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

    Timing::Initialize();

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

        TimeStats::PreRender();

        /*
         * Force a Hourglass frame boundary.
         */
        HDC hdc = GetDC(hWnd);
        SwapBuffers(hdc);
        ReleaseDC(hWnd, hdc);

        TimeStats::PostRender();
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
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

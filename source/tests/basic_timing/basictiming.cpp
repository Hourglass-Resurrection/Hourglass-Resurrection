/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include "stdafx.h"
#include "logger.h"
#include "time_stats.h"

constexpr WCHAR CLASS_NAME[] = L"basic_timing";
constexpr WCHAR WINDOW_NAME[] = L"basic_timing";

static uint32_t gs_pixel = 0x000000CC;
static BITMAPINFO gs_bitmap_info;

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

    {
        BITMAPINFOHEADER& header = gs_bitmap_info.bmiHeader;
        header.biSize = sizeof(header);
        header.biWidth = 1;
        header.biHeight = 1;
        header.biPlanes = 1;
        header.biBitCount = 32;
        header.biCompression = BI_RGB;
    }

    TimeStats::Initialize();

    MSG msg;

    // Main message loop:
    bool running = true;
    while (running)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = false;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        TimeStats::PreRender();

        HDC hdc = GetDC(hWnd);
        RECT client_rect;
        GetClientRect(hWnd, &client_rect);
        StretchDIBits(hdc,
                      0, 0, client_rect.right, client_rect.bottom,
                      0, 0, 1, 1,
                      &gs_pixel,
                      &gs_bitmap_info,
                      DIB_RGB_COLORS,
                      SRCCOPY);
        /*
         * Force a Hourglass frame boundary.
         */
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
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT client_rect;
            GetClientRect(hWnd, &client_rect);

            StretchDIBits(hdc,
                          0, 0, client_rect.right, client_rect.bottom,
                          0, 0, 1, 1,
                          &gs_pixel,
                          &gs_bitmap_info,
                          DIB_RGB_COLORS,
                          SRCCOPY);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

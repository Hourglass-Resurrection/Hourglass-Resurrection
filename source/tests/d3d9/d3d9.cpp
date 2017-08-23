/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include "stdafx.h"
#include "d3d_stuff.h"
#include "logger.h"
#include "time_stats.h"

constexpr WCHAR CLASS_NAME[] = L"d3d9";
constexpr WCHAR WINDOW_NAME[] = L"d3d9";

ATOM             MyRegisterClass(HINSTANCE);
BOOL             InitInstance(HINSTANCE, int);
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
    if (!InitInstance(hInstance, nCmdShow))
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
                running = false;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        TimeStats::PreRender();
        D3D::RenderFrame();
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

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
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
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    try
    {
        D3D::Initialize(hWnd);
    }
    catch (const D3D::Error::CreateD3D&)
    {
        Logger::WriteLine(L"Error in Direct3DCreate9().");
        return FALSE;
    }
    catch (const D3D::Error::CreateDevice& e)
    {
        std::wstring error_string;
        switch (e.m_return_code)
        {
        case D3DERR_DEVICELOST:
            error_string = L"D3DERR_DEVICELOST";
            break;
        case D3DERR_INVALIDCALL:
            error_string = L"D3DERR_INVALIDCALL";
            break;
        case D3DERR_NOTAVAILABLE:
            error_string = L"D3DERR_NOTAVAILABLE";
            break;
        case D3DERR_OUTOFVIDEOMEMORY:
            error_string = L"D3DERR_OUTOFVIDEOMEMORY";
            break;
        default:
            error_string = std::to_wstring(e.m_return_code);
            break;
        }

        Logger::WriteLine(L"Error in CreateDevice(): " + error_string + L'.');
        return FALSE;
    }

    return TRUE;
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

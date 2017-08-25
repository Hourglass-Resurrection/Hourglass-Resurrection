/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include "stdafx.h"
#include "d3d_stuff.h"

namespace D3D
{
    namespace
    {
        LPDIRECT3D9EX gs_d3d;
        LPDIRECT3DDEVICE9 gs_device;
    }

    void Initialize(HWND hwnd)
    {
        HRESULT result = Direct3DCreate9Ex(D3D_SDK_VERSION, &gs_d3d);

        if (FAILED(result))
        {
            throw Error::CreateD3D{ result };
        }

        D3DPRESENT_PARAMETERS params;

        ZeroMemory(&params, sizeof(params));
        params.Windowed = TRUE;
        params.SwapEffect = D3DSWAPEFFECT_DISCARD;
        params.hDeviceWindow = hwnd;

        /*
         * TODO: Replace with CreateDeviceEx() when hooks are ready.
         * -- YaLTeR
         */
        result = gs_d3d->CreateDevice(D3DADAPTER_DEFAULT,
                                      D3DDEVTYPE_HAL,
                                      hwnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &params,
                                      &gs_device);

        if (FAILED(result))
        {
            gs_d3d->Release();
            throw Error::CreateDevice{ result };
        }
    }

    void Shutdown()
    {
        gs_d3d->Release();
        gs_device->Release();
    }

    void RenderFrame()
    {
        gs_device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 200), 1.0f, 0);
        gs_device->Present(nullptr, nullptr, nullptr, nullptr);
    }
}

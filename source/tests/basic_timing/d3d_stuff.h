#pragma once

namespace D3D
{
    namespace Error
    {
        struct CreateD3D {};

        struct CreateDevice
        {
            HRESULT m_return_code;
        };
    }

    void Initialize(HWND hwnd);
    void Shutdown();
    void RenderFrame();
}

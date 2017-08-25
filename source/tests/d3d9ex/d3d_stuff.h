/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

namespace D3D
{
    namespace Error
    {
        struct CreateD3D
        {
            HRESULT m_return_code;
        };

        struct CreateDevice
        {
            HRESULT m_return_code;
        };
    }

    void Initialize(HWND hwnd);
    void Shutdown();
    void RenderFrame();
}

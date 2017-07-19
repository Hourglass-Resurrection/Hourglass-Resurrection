#pragma once

#include <Windows.h>

#include "shared/ipc.h"

namespace IPC
{
    void SendIPCMessage(Command cmd, LPCVOID data, DWORD data_size);

    /*
     * HACK: Call only once from DllMain!
     * TODO: Remove this call when the new DLL loading is in place.
     * -- Warepire
     */
    void SendIPCBufferAddress();
}

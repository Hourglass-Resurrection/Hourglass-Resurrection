#include <Windows.h>

#include "shared/ipc.h"

#include "ipc.h"

namespace IPCInternal
{
    static IPC::CommandFrame cmd_frame;
    static volatile LONG ipc_lock;
}

namespace IPC
{
    /*
     * TODO: Reply checking...
     */
    void SendIPCMessage(Command cmd, LPCVOID data, DWORD data_size)
    {
        while (InterlockedBitTestAndSet(&IPCInternal::ipc_lock, 0)) {}
        IPCInternal::cmd_frame = { cmd, data_size, data, };
        _asm { int 3 };
        InterlockedExchange(&IPCInternal::ipc_lock, 0);
    }

    void SendIPCBufferAddress()
    {
        WCHAR buf[128];
        swprintf(buf, ARRAYSIZE(buf), L"IPCBUFFER: %p", &IPCInternal::cmd_frame);
        OutputDebugStringW(buf);
    }
}

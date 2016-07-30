#pragma once

#include <Windows.h>

#include "shared/ipc.h"

namespace IPC
{
    void SendIPCMessage(Command cmd, LPVOID data, DWORD data_size);
}

#pragma once

#include <Windows.h>

#include "shared/ipc.h"

namespace IPC
{
    void SendIPCMessage(Command cmd, LPCVOID data, DWORD data_size);
}

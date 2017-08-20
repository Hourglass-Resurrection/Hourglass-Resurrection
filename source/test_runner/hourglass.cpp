#include <filesystem>
#include <iostream>
#include <string>

#include "windows.h"

namespace Hourglass
{
    namespace filesystem = std::experimental::filesystem;

    namespace
    {
        filesystem::path gs_hourglass_exe;
    }

    bool Find(const std::experimental::filesystem::path& base_path)
    {
        gs_hourglass_exe = base_path.parent_path().parent_path() / "Debug" / "hourglass-resurrection-d.exe";

        if (!filesystem::exists(gs_hourglass_exe))
        {
            std::wcout << L"Could not find the Hourglass .exe at " << gs_hourglass_exe << L". Quitting.\n";
            return false;
        }
        else
        {
            std::wcout << L"Using the Hourglass .exe at " << gs_hourglass_exe << L'\n';
            return true;
        }
    }

    void Run(const filesystem::path& exe, const filesystem::path& input)
    {
        std::wstring commandline = L'"' + gs_hourglass_exe.wstring()
                                 + L"\" --game \"" + exe.wstring()
                                 + L"\" --input \"" + input.wstring()
                                 + L"\" --play";

        std::wcout << commandline << L'\n';

        STARTUPINFOW si{};
        si.cb = sizeof(si);

        PROCESS_INFORMATION pi{};

        /*
         * According to MSDN, CreateProcessW() can modify the commandline argument.
         */
        if (CreateProcessW(nullptr,
                           commandline.data(),
                           nullptr,
                           nullptr,
                           FALSE,
                           0,
                           nullptr,
                           gs_hourglass_exe.parent_path().c_str(),
                           &si,
                           &pi) == 0)
            throw std::runtime_error("Error starting Hourglass: " + std::to_string(GetLastError()));

        CloseHandle(pi.hThread);

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
    }
}

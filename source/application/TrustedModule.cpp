/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "Config.h"

#include "TrustedModule.h"

namespace
{
    /*
     * This is rougly based on the contents of my SysWOW64 folder.
     * -- YaLTeR
     */
    constexpr const WCHAR* CRT_FILENAMES[] = {
        L"msvcrt.dll",
        L"msvcrt20.dll",
        L"msvcrt40.dll",

        L"msvcp60.dll",
        L"msvcp60d.dll",
        L"msvcp100.dll",
        L"msvcp100d.dll",
        L"msvcp120.dll",
        L"msvcp120d.dll",
        L"msvcp140.dll",
        L"msvcp140d.dll",

        L"msvcr100.dll",
        L"msvcr100d.dll",
        L"msvcr120.dll",
        L"msvcr120d.dll",
    };

    std::map<DWORD, std::vector<std::wstring>> s_trusted_modules;

    size_t CountSharedPrefixLength(const std::wstring& a, const std::wstring& b)
    {
        size_t shortest_path = std::min(a.size(), b.size());
        size_t i;
        for (i = 0; i < shortest_path && tolower(a[i]) == tolower(b[i]); ++i)
        {
        }
        return i;
    }

    UINT CountBackslashes(const std::wstring& str)
    {
        return std::count(str.begin(), str.end(), L'\\');
    }

    std::wstring GetFilenameWithoutPath(const std::wstring& path)
    {
        size_t last = path.find_last_of(L"\\/");
        if (last == std::wstring::npos)
        {
            return path;
        }
        return path.substr(last + 1);
    }
}

/*
 * Hacky... yucky
 * -- Warepire
 */
extern std::wstring injected_dll_path;
extern std::wstring sub_exe_filename;

namespace TrustedModule
{
    void OnLoad(DWORD process_id, const std::wstring& path)
    {
        /*
         * If we can "trust" a newly loaded module, add it to the list.
         */
        UINT proximity = 2;
        if (path == injected_dll_path)
        {
            s_trusted_modules[process_id].emplace_back(path);
        }
        else if (CountBackslashes(Config::exe_filename.c_str() + CountSharedPrefixLength(Config::exe_filename, path)) < proximity)
        {
            s_trusted_modules[process_id].emplace_back(path);
        }
        else if (CountBackslashes(sub_exe_filename.c_str() + CountSharedPrefixLength(sub_exe_filename, path)) < proximity)
        {
            s_trusted_modules[process_id].emplace_back(path);
        }
        else if (path.length() >= 4 && (path.substr(path.length() - 4)) == L".cox") // hack, we can generally assume the game outputted any dll that has this extension
        {
            s_trusted_modules[process_id].emplace_back(path);
        }
        else
        {
            std::wstring filename = GetFilenameWithoutPath(path);

            if (std::find_if(std::cbegin(CRT_FILENAMES),
                             std::cend(CRT_FILENAMES),
                             [&filename](const std::wstring& m) {
                                 return (filename == m);
                             }) != std::cend(CRT_FILENAMES))
            {
                s_trusted_modules[process_id].emplace_back(path);
            }
        }
    }

    void OnUnload(DWORD process_id, const std::wstring& path)
    {
        auto& proc_modules = s_trusted_modules[process_id];
        auto removed = std::remove_if(proc_modules.begin(), proc_modules.end(),
                                      [&path](const std::wstring& m) { return (m == path); });

        if (removed == proc_modules.end())
        {
            return;
        }
        proc_modules.erase(removed, proc_modules.end());
    }

    bool IsTrusted(DWORD process_id, const std::wstring& path)
    {
        auto CheckModule = [&path](const std::wstring &m) {
            return (path == m);
        };
        return (std::find_if(s_trusted_modules[process_id].begin(),
                             s_trusted_modules[process_id].end(),
                             CheckModule) != s_trusted_modules[process_id].end());
    }
}

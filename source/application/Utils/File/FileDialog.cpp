﻿/*
 * Copyright(c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommDlg.h>

#include <algorithm>
#include <map>

#include "../File.h"

namespace
{
    /*
     * The pipe characters are just placeholders for the null characters to ease string manipulation
     */
    std::map<Utils::File::FileFilter, LPCWSTR> file_type_map =
        {{Utils::File::FileFilter::AllFiles, L"All Files (*.*)|*.*|"},
         {Utils::File::FileFilter::HourglassMovie, L"Windows TAS Files (*.hgr)|*.hgr|"},
         {Utils::File::FileFilter::Executable, L"Executables (*.exe)|*.exe|"},
         {Utils::File::FileFilter::Config, L"Config Files (*.cfg)|*.cfg|"},
         {Utils::File::FileFilter::WatchList, L"Watchlist (*.wch)|*.wch|"},
         {Utils::File::FileFilter::AVI, L"AVI file (*.avi)|*.avi|"}};

    std::wstring GetFilter(const std::vector<Utils::File::FileFilter>& file_types)
    {
        std::wstring result;

        for (auto& file_type : file_types)
        {
            LPCWSTR filter = file_type_map[file_type];
            result.append(filter);
        }

        std::replace(result.begin(), result.end(), L'|', L'\0');
        return result;
    }

    using FileNameGetter = BOOL(WINAPI*)(LPOPENFILENAMEW open_file_name);

    std::wstring GetFileName(const std::wstring& location,
                             const std::vector<Utils::File::FileFilter>& file_types,
                             const DWORD& flags,
                             const FileNameGetter file_name_getter)
    {
        auto filter = GetFilter(file_types);

        WCHAR buffer[FILENAME_MAX] = {0};
        location.copy(buffer, location.length());

        OPENFILENAMEW open_file_name = {0};
        open_file_name.lStructSize = sizeof(open_file_name);
        open_file_name.lpstrFilter = filter.data();
        open_file_name.nFilterIndex = 1;
        open_file_name.lpstrFile = buffer;
        open_file_name.nMaxFile = FILENAME_MAX;
        open_file_name.lpstrInitialDir = location.c_str();
        open_file_name.Flags = flags;

        return file_name_getter(&open_file_name) ? buffer : L"";
    }
}

std::wstring Utils::File::GetFileNameOpen(const std::wstring& location,
                                          const std::vector<Utils::File::FileFilter>& file_types)
{
    auto flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    return GetFileName(location, file_types, flags, GetOpenFileName);
}

std::wstring Utils::File::GetFileNameSave(const std::wstring& location,
                                          const std::vector<Utils::File::FileFilter>& file_types)
{
    auto flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOREADONLYRETURN;
    return GetFileName(location, file_types, flags, GetSaveFileName);
}

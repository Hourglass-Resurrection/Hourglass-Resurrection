/*
 * Copyright(c) 2017 - Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <windows.h>
#include <CommDlg.h>

#include <map>
#include <algorithm>

#include "../File.h"

namespace
{
    // The pipe characters are just placeholders for the null characters to ease string manipulation
    std::map<Utils::File::FileFilter, const char*> gs_file_type_map =
    {
        { Utils::File::FileFilter::AllFiles, "All Files (*.*)|*.*|" },
        { Utils::File::FileFilter::HourglassMovie, "Windows TAS Files (*.hgr)|*.hgr|" },
        { Utils::File::FileFilter::Executable, "Executables (*.exe)|*.exe|" },
        { Utils::File::FileFilter::Config, "Config Files (*.cfg)|*.cfg|" }
    };
    
    std::string GetFilter(const std::vector<Utils::File::FileFilter> file_types)
    {
        std::string result = "";

        for (auto file_type : file_types)
        {
            const char* filter = gs_file_type_map[file_type];
            result.append(filter);
        }

        std::replace(result.begin(), result.end(), '|', '\0');
        return result;
    }
}

std::string Utils::File::GetFileNameOpen(
    const std::string location,
    const std::vector<File::FileFilter> file_types)
{
    auto filter = GetFilter(file_types);

    char buffer[FILENAME_MAX] = { 0 };
    location.copy(buffer, location.length());

    OPENFILENAME open_file_name = { 0 };
    open_file_name.lStructSize = sizeof(open_file_name);
    open_file_name.lpstrFilter = filter.data();
    open_file_name.nFilterIndex = 1;
    open_file_name.lpstrFile = buffer;
    open_file_name.nMaxFile = FILENAME_MAX;
    open_file_name.lpstrInitialDir = location.c_str();
    open_file_name.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    return GetOpenFileName(&open_file_name) ? buffer : "";
}

std::string Utils::File::GetFileNameSave(
    const std::string location,
    const std::vector<File::FileFilter> file_types)
{
    auto filter = GetFilter(file_types);

    char buffer[FILENAME_MAX] = { 0 };
    location.copy(buffer, location.length());

    OPENFILENAME open_file_name = { 0 };
    open_file_name.lStructSize = sizeof(open_file_name);
    open_file_name.lpstrFilter = filter.data();
    open_file_name.nFilterIndex = 1;
    open_file_name.lpstrFile = buffer;
    open_file_name.nMaxFile = FILENAME_MAX;
    open_file_name.lpstrInitialDir = location.c_str();
    open_file_name.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    return GetSaveFileName(&open_file_name) ? buffer : "";
}

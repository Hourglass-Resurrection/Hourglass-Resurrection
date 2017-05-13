/*
 * Copyright(c) 2017 - Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#include <string>
#include <vector>

namespace Utils
{
    namespace File
    {
        enum class FileFilter
        {
            AllFiles,
            HourglassMovie,
            Executable,
            Config
        };

        std::string GetFileNameOpen(
            const std::string location,
            const std::vector<FileFilter> file_types);

        std::string GetFileNameSave(
            const std::string location,
            const std::vector<FileFilter> file_types);

        std::string GetFileName(std::string path);
        std::string GetDirectoryName(std::string path);
    }
}

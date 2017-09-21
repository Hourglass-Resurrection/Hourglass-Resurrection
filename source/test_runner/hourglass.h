/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once
#include <filesystem>

namespace Hourglass
{
    bool Find(const std::experimental::filesystem::path& base_path);
    void Run(const std::experimental::filesystem::path& exe,
             const std::experimental::filesystem::path& input);
}

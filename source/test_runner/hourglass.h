#pragma once
#include <filesystem>

namespace Hourglass
{
    bool Find(const std::experimental::filesystem::path& base_path);
    void Run(const std::experimental::filesystem::path& exe, const std::experimental::filesystem::path& input);
}

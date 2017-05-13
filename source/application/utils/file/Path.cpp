/*
 * Copyright(c) 2017 - Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include "../File.h"

std::string Utils::File::GetFileName(const std::string path)
{
    size_t last_slash = path.find_last_of("/\\");
    return last_slash == std::string::npos ? path : path.substr(last_slash + 1);
}

std::string Utils::File::GetDirectoryName(const std::string path)
{
    size_t last_slash = path.find_last_of("/\\");
    return last_slash == std::string::npos ? std::string() : path.substr(0, last_slash);
}

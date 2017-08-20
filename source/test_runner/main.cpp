/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <filesystem>

#include <io.h>
#include <fcntl.h>

#define CATCH_CONFIG_RUNNER
#include "Catch/catch.hpp"

#include "binary_tests.h"
#include "hourglass.h"

int main(int argc, char* argv[])
{
    int previous_mode = _setmode(_fileno(stdout), _O_WTEXT);

    /*
     * Figure out the base path.
     */
    WCHAR test_runner_exe_filename[MAX_PATH];
    GetModuleFileNameW(nullptr, test_runner_exe_filename, ARRAYSIZE(test_runner_exe_filename));
    std::wcout << L"Test runner .exe filename: " << test_runner_exe_filename << L'\n';

    const std::experimental::filesystem::path test_runner_exe(test_runner_exe_filename);
    const std::experimental::filesystem::path base_path = test_runner_exe.parent_path().parent_path();

    if (!Hourglass::Find(base_path))
        return 1;

    BinaryTests::DiscoverTests(base_path);

    _setmode(_fileno(stdout), previous_mode);

    int result = Catch::Session().run(argc, argv);
    return (result < 0xff ? result : 0xff);
}

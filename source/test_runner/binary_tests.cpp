#include <filesystem>
#include <iostream>
#include <vector>

#include "windows.h"

#include "catch.hpp"

namespace BinaryTests
{
    namespace filesystem = std::experimental::filesystem;

    namespace
    {
        std::vector<filesystem::directory_entry> test_folders;
    }

    void DiscoverTests()
    {
        WCHAR exe_filename[MAX_PATH];
        GetModuleFileNameW(nullptr, exe_filename, ARRAYSIZE(exe_filename));
        std::wcout << L"Exe filename: " << exe_filename << L'\n';

        const filesystem::path exe_path(exe_filename);
        const filesystem::path binary_test_folder = exe_path.parent_path().parent_path() / "bin";

        for (auto entry : filesystem::directory_iterator(binary_test_folder))
        {
            if (filesystem::is_directory(entry))
            {
                std::wcout << L"Discovered binary test folder: " << entry.path().filename().c_str() << L'\n';

                test_folders.push_back(std::move(entry));
            }
        }

        std::wcout << L'\n';
    }

    TEST_CASE( "binary tests" )
    {
        for (auto&& folder : test_folders)
        {
            SECTION( folder.path().filename().string() )
            {
                REQUIRE( 1 == 1 );
            }
        }
    }
}

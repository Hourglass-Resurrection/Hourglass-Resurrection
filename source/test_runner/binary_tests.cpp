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
        constexpr char TEMP_DIR_NAME[] = "temp";

        filesystem::path base_path;
        std::vector<filesystem::directory_entry> test_folders;

        class TempDir
        {
        public:
            TempDir()
            {
                filesystem::create_directory(base_path / TEMP_DIR_NAME);
            }

            ~TempDir()
            {
                filesystem::remove_all(base_path / TEMP_DIR_NAME);
            }
        };

        void CopyContentsToTempDir(const filesystem::path& folder)
        {
            filesystem::copy(folder, base_path / TEMP_DIR_NAME, filesystem::copy_options::recursive);
        }

        TEST_CASE( "binary tests" )
        {
            TempDir temp_dir;

            for (auto&& folder : test_folders)
            {
                SECTION( folder.path().filename().string() )
                {
                    CopyContentsToTempDir(folder);

                    REQUIRE( 1 == 1 );
                }
            }
        }
    }

    void DiscoverTests()
    {
        WCHAR exe_filename[MAX_PATH];
        GetModuleFileNameW(nullptr, exe_filename, ARRAYSIZE(exe_filename));
        std::wcout << L"Exe filename: " << exe_filename << L'\n';

        const filesystem::path exe_path(exe_filename);
        base_path = exe_path.parent_path().parent_path();

        for (auto entry : filesystem::directory_iterator(base_path / "bin"))
        {
            if (filesystem::is_directory(entry))
            {
                std::wcout << L"Discovered binary test folder: " << entry.path().filename().c_str() << L'\n';

                test_folders.push_back(std::move(entry));
            }
        }

        std::wcout << L'\n';
    }
}

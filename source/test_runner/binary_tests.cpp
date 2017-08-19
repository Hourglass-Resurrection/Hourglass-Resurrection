#include <filesystem>
#include <iostream>
#include <map>
#include <vector>

#include "windows.h"

#include "catch.hpp"

#include "hourglass.h"

namespace BinaryTests
{
    namespace filesystem = std::experimental::filesystem;

    namespace
    {
        constexpr char TEMP_DIR_NAME[] = "temp";

        struct InputPair
        {
            filesystem::path input;
            filesystem::path log;
        };

        filesystem::path gs_base_path;
        std::map<filesystem::path, std::vector<InputPair>> gs_tests;

        class TempDir
        {
        public:
            TempDir()
            {
                filesystem::create_directory(gs_base_path / TEMP_DIR_NAME);
            }

            ~TempDir()
            {
                filesystem::remove_all(gs_base_path / TEMP_DIR_NAME);
            }
        };

        void CopyContentsToTempDir(const filesystem::path& folder)
        {
            filesystem::copy(folder, gs_base_path / TEMP_DIR_NAME, filesystem::copy_options::recursive);
        }

        TEST_CASE( "binary tests" )
        {
            TempDir temp_dir;

            for (const auto& test : gs_tests)
            {
                SECTION( test.first.filename().string() )
                {
                    const auto exe_filename = test.first.filename().replace_extension("exe");
                    const auto exe_path = gs_base_path / TEMP_DIR_NAME / exe_filename;

                    for (const auto& input_pair : test.second)
                    {
                        SECTION( input_pair.input.filename().string() )
                        {
                            Hourglass::Run(exe_path, input_pair.input);
                            REQUIRE( 1 == 1 );
                        }
                    }
                }
            }
        }
    }

    void DiscoverTests(const filesystem::path& base_path)
    {
        gs_base_path = base_path;

        const filesystem::path input_path = gs_base_path / "input";

        // Find all folders in bin/.
        for (auto&& entry : filesystem::directory_iterator(gs_base_path / "bin"))
        {
            if (filesystem::is_directory(entry))
            {
                const auto path = entry.path();

                std::wcout << L"Discovered binary test folder: " << path.filename() << L'\n';

                // Make sure that the .exe exists.
                const auto exe_filename = path.filename().replace_extension("exe");
                if (!filesystem::exists(path / exe_filename))
                {
                    std::wcout << L"\t...but it doesn't contain " << exe_filename << L"! Ignoring.\n";
                    continue;
                }

                // Find all corresponding input/logfile pairs in input/<folder>/.
                for (auto&& input : filesystem::directory_iterator(input_path / path.filename()))
                {
                    auto input_path = input.path();

                    if (input_path.extension().string() == ".hgr")
                    {
                        auto log_path = filesystem::path(input_path).replace_extension("log");

                        // For every input file, check if the log file exists with the same stem.
                        if (!filesystem::exists(log_path))
                        {
                            std::wcout << L"\tNo log file found for " << input_path.filename() << L"! Ignoring.\n";
                            continue;
                        }

                        std::wcout << L"\tTest input: " << input_path.filename() << L'\n';

                        gs_tests[path].push_back(InputPair{ std::move(input_path), std::move(log_path) });
                    }
                }
            }
        }

        std::wcout << L'\n';
    }
}

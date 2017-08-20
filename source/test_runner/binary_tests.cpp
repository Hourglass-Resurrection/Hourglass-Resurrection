#include <filesystem>
#include <fstream>
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
        constexpr char OUTPUT_LOG_FILENAME[] = "output.log";
        constexpr char FAILED_DIR_NAME[] = "failed";

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

        std::vector<std::wstring> ReadLog(const filesystem::path& path)
        {
            std::vector<std::wstring> rv;

            std::wifstream file(path);
            std::wstring line;
            while (std::getline(file, line))
                rv.push_back(std::move(line));

            return rv;
        }

        bool CompareLogs(const filesystem::path& expected, const filesystem::path& output)
        {
            auto expected_lines = ReadLog(expected);
            auto output_lines = ReadLog(output);

            bool logs_match = true;

            if (expected_lines.size() == output_lines.size())
            {
                for (size_t i = 0; i < expected_lines.size(); ++i)
                {
                    auto expected_line = expected_lines[i];
                    auto output_line = output_lines[i];

                    if (expected_line != output_line)
                    {
                        std::wcout << L"Log mismatch detected on line " << i
                                   << L":\nExpected: " << expected_line
                                   << L"\nOutput:   " << output_line << L'\n';

                        return false;
                    }
                }
            }
            else
            {
                std::wcout << L"The log has a wrong line count. Expected: " << expected_lines.size()
                           << L"; got: " << output_lines.size() << L'\n';

                return false;
            }

            return true;
        }

        TEST_CASE( "binary tests" )
        {
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
                            /*
                             * Create a temp folder and copy the test there.
                             */
                            TempDir temp_dir;
                            CopyContentsToTempDir(test.first);

                            /*
                             * Run Hourglass on the test .exe with the current input file.
                             */
                            REQUIRE_NOTHROW( Hourglass::Run(exe_path, input_pair.input) );

                            /*
                             * Make sure the test output its log file.
                             */
                            const filesystem::path output_log = gs_base_path / TEMP_DIR_NAME / OUTPUT_LOG_FILENAME;
                            REQUIRE( filesystem::exists(output_log) );

                            /*
                             * Compare the test output with the expected output.
                             */
                            const bool logs_match = CompareLogs(input_pair.log, output_log);

                            /*
                             * If the logs do not match, save the output log for further inspection.
                             */
                            if (!logs_match)
                            {
                                const filesystem::path failed = gs_base_path / FAILED_DIR_NAME / test.first.filename();
                                filesystem::create_directories(failed);

                                filesystem::path expected = failed / input_pair.log.filename();
                                expected.replace_filename(expected.stem().wstring() + L"_expected.log");

                                filesystem::path output = failed / (input_pair.log.stem().wstring() + L"_output.log");

                                filesystem::copy_file(input_pair.log, expected);
                                filesystem::copy_file(output_log, output);
                            }

                            REQUIRE( logs_match );
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

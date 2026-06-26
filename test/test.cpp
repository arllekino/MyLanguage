#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <catch2/catch_all.hpp>

namespace fs = std::filesystem;

void Execute(const std::string& executorPath, const std::string& testPath)
{
    const auto executablePath = executorPath + " " + testPath;
    system(executablePath.c_str());
}

TEST_CASE("Language Tests")
{
    const std::string executor = "/Users/miroslav.aktuganov/CLionProjects/MyLanguage/cmake-build-release/MyLanguage";
    const std::string testPath = "/Users/miroslav.aktuganov/CLionProjects/MyLanguage/test";

    if (fs::exists(testPath) && fs::is_directory(testPath))
    {
        for (const auto& entry : fs::recursive_directory_iterator(testPath))
        {
            const std::string filePath = entry.path().string();

            if (fs::is_regular_file(entry) && filePath.ends_with(".txt"))
            {
                std::cout << "===  " << filePath << "   ===" << std::endl;
                Execute(executor, entry.path());
                std::cout << "\n";
            }
        }
    }
}
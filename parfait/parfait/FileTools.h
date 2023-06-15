#pragma once
#include <vector>
#include <string>

namespace Parfait {
namespace FileTools {
    std::string readNextLine(FILE* fp);
    bool readNextLine(FILE* fp, std::string& string_line);
    std::string readLinesInFile(FILE* fp, int n);
    long countLinesInFile(std::string filename);
    std::string loadFileToString(std::string filename);

    void writeStringToFile(std::string filename, std::string contents);

    void appendToFile(std::string filename, std::string message);

    bool doesDirectoryExist(std::string directory);

    bool doesFileExist(std::string filename);

    template <typename MP>
    bool doesFileExist(MP mp, std::string filename) {
        bool good = false;
        if (mp.Rank() == 0) {
            good = doesFileExist(filename);
        }
        mp.Broadcast(good, 0);
        return good;
    }

    std::vector<std::string> listFilesInPath(const std::string& path);
    void waitForFile(std::string filename, int max_seconds = 10);
}
}

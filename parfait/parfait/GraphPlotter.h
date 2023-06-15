#pragma once
#include <string>
#include <cstdio>
#include <map>
#include <set>
#include <vector>
#include "Throw.h"
#include "FileTools.h"
#include "StringTools.h"

namespace Parfait {
namespace Graph {

    template <typename Container>
    std::map<long, std::set<long>> toSparse(const std::vector<Container>& dense) {
        std::map<long, std::set<long>> sparse;
        for (long r = 0; r < long(dense.size()); r++) {
            if (dense[r].size() > 0) {
                for (auto c : dense[r]) {
                    sparse[r].insert(c);
                }
            }
        }
        return sparse;
    }

    inline std::vector<std::vector<long>> toDense(const std::map<long, std::set<long>>& sparse) {
        long biggest_row = std::numeric_limits<long>::lowest();

        for (auto& pair : sparse) {
            auto row = pair.first;
            biggest_row = std::max(biggest_row, row);
        }

        std::vector<std::vector<long>> dense(biggest_row);
        for (long r = 0; r < biggest_row; r++) {
            if (sparse.count(r)) {
                for (auto c : sparse.at(r)) {
                    dense[r].push_back(c);
                }
            }
        }
        return dense;
    }

    inline std::map<long, std::set<long>> read(std::string filename) {
        FILE* fp = fopen(filename.c_str(), "r");
        PARFAIT_ASSERT(fp != nullptr, "Could not open file for reading: " + filename);

        std::map<long, std::set<long>> graph_read;

        std::string line;
        while (Parfait::FileTools::readNextLine(fp, line)) {
            auto words = Parfait::StringTools::split(line, " ");
            if (words.size() >= 2) {
                long row = Parfait::StringTools::toLong(Parfait::StringTools::strip(words[0]));
                long col = Parfait::StringTools::toLong(Parfait::StringTools::strip(words[1]));
                graph_read[row].insert(col);
            }
        }
        fclose(fp);

        return graph_read;
    }

    inline std::map<long, std::set<std::pair<long, double>>> readMatrix(std::string filename) {
        FILE* fp = fopen(filename.c_str(), "r");
        PARFAIT_ASSERT(fp != nullptr, "Could not open file for reading: " + filename);

        std::map<long, std::set<std::pair<long, double>>> graph_read;

        std::string line;
        while (Parfait::FileTools::readNextLine(fp, line)) {
            auto words = Parfait::StringTools::split(line, " ");
            if (words.size() >= 2) {
                long row = Parfait::StringTools::toLong(Parfait::StringTools::strip(words[0]));
                long col = Parfait::StringTools::toLong(Parfait::StringTools::strip(words[1]));
                long data = Parfait::StringTools::toDouble(Parfait::StringTools::strip(words[2]));
                graph_read[row].insert(std::make_pair(col, data));
            }
        }
        fclose(fp);

        return graph_read;
    }

    inline std::vector<std::vector<long>> readDense(std::string filename) { return toDense(read(filename)); }

    inline void write(std::string filename, const std::vector<std::vector<long>> graph, std::string file_mode = "w") {
        FILE* fp = fopen(filename.c_str(), file_mode.c_str());
        for (unsigned long row = 0; row < graph.size(); row++) {
            for (unsigned long col : graph[row]) {
                fprintf(fp, "%lu %lu\n", row, col);
            }
        }
        fclose(fp);
    }
    inline void write(std::string filename, const std::map<long, std::set<long>>& graph, std::string file_mode = "w") {
        FILE* fp = fopen(filename.c_str(), file_mode.c_str());
        PARFAIT_ASSERT(fp != nullptr, "Could not open file for writing: " + filename);

        for (auto& pair : graph) {
            auto row = pair.first;
            auto& columns = pair.second;
            for (auto c : columns) {
                fprintf(fp, "%ld %ld\n", row, c);
            }
        }
        fclose(fp);
    }
    inline void write(std::string filename,
                      const std::map<long, std::set<std::pair<long, double>>>& graph,
                      std::string file_mode = "w") {
        FILE* fp = fopen(filename.c_str(), file_mode.c_str());
        PARFAIT_ASSERT(fp != nullptr, "Could not open file for writing: " + filename);

        for (auto& pair : graph) {
            auto row = pair.first;
            auto& columns = pair.second;
            for (auto c : columns) {
                fprintf(fp, "%ld %ld %e\n", row, c.first, c.second);
            }
        }
        fclose(fp);
    }
    inline void writeGnuplot(std::string filename, const std::vector<std::vector<long>>& graph) {
        return write(filename, graph);
    }
    inline void writeGnuplot(std::string filename, const std::map<long, std::set<long>>& graph) {
        return write(filename, toDense(graph));
    }
    inline void writeGnuplot(std::string filename, const std::vector<std::set<long>>& graph) {
        return writeGnuplot(filename, toSparse(graph));
    }

}
}

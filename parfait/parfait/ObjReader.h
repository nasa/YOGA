#pragma once
#include "Facet.h"
#include "FileTools.h"
#include "StringTools.h"

namespace Parfait {
class ObjWriter {
  public:
    static void write(std::string filename, const std::vector<Parfait::Facet>& facets) {
        FILE* fp = fopen(filename.c_str(), "w");

        fprintf(fp, "#%s\n", filename.c_str());

        for (auto& f : facets) {
            for (int i = 0; i < 3; i++) {
                fprintf(fp, "v %s\n", f[i].to_string().c_str());
            }
        }

        int offset = 1;  // obj indices start at 1
        for (int f = 0; f < int(facets.size()); f++) {
            int n1 = 3 * f + 0 + offset;
            int n2 = 3 * f + 1 + offset;
            int n3 = 3 * f + 2 + offset;
            fprintf(fp, "f %d %d %d\n", n1, n2, n3);
        }

        fclose(fp);
    }
};
class ObjReader {
  public:
    static std::vector<Parfait::Facet> read(std::string filename) {
        ObjReader reader(filename);
        return reader.read();
    }

    ObjReader(std::string filename) : filename(filename) {}
    void parseLine(std::string& line) {
        line = Parfait::StringTools::strip(line);

        if (line.empty()) return;
        if (Parfait::StringTools::startsWith(line, "#")) return;

        auto words = Parfait::StringTools::split(line, " ");
        auto first_word = words.front();
        if (first_word == "f") {
            if (words.size() != 4) {
                PARFAIT_WARNING("Could not parse line in obj file we think is a polygon(triangle): " + line);
            }
            std::array<std::string, 3> node_words{words[1], words[2], words[3]};
            int offset = 1;  // obj indices start at 1
            std::array<int, 3> tri;
            for (int i = 0; i < 3; i++) {
                node_words[i] = Parfait::StringTools::split(node_words[i], "/").front();
                tri[i] = Parfait::StringTools::toInt(node_words[i]) - offset;
                PARFAIT_ASSERT(tri[i] >= 0, "Negative nodes need to be wrapped back around (like python indices)");
            }
            triangles.push_back(tri);
        } else if (first_word == "v") {
            if (words.size() != 4) {
                PARFAIT_WARNING("Could not parse line in obj file we think is a vertex: " + line);
            }
            double x = Parfait::StringTools::toDouble(words[1]);
            double y = Parfait::StringTools::toDouble(words[2]);
            double z = Parfait::StringTools::toDouble(words[3]);
            points.push_back({x, y, z});
        }
    }
    std::vector<Parfait::Facet> read() {
        FILE* fp = fopen(filename.c_str(), "r");
        PARFAIT_ASSERT(fp != nullptr, "Could not open file for reading " + filename);

        std::string line;
        while (Parfait::FileTools::readNextLine(fp, line)) {
            parseLine(line);
        }
        fclose(fp);

        std::vector<Parfait::Facet> facets(triangles.size());
        for (long t = 0; t < long(triangles.size()); t++) {
            auto& tri = triangles[t];
            facets[t] = {points.at(tri.at(0)), points.at(tri.at(1)), points.at(tri.at(2))};
        }
        return facets;
    }

  public:
    std::string filename;
    std::vector<Parfait::Point<double>> points;
    std::vector<std::array<int, 3>> triangles;
};

}

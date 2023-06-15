#pragma once
#include <parfait/Point.h>
#include <parfait/FileTools.h>
#include <parfait/StringTools.h>

namespace Parfait {
class PointwiseSources {
  public:
    inline PointwiseSources(const std::vector<Parfait::Point<double>>& points,
                            const std::vector<double>& spacing,
                            const std::vector<double>& decay)
        : points(points), spacing(spacing), decay(decay) {}

    inline PointwiseSources(const std::string& filename) {
        FILE* f = fopen(filename.c_str(), "r");
        readBody(f);
        fclose(f);
    }

    inline void write(const std::string& filename) {
        FILE* f = fopen(filename.c_str(), "w");
        writeHeader(f);
        writeBody(f);
        fclose(f);
    }

    std::vector<Parfait::Point<double>> points;
    std::vector<double> spacing;
    std::vector<double> decay;

  private:
    inline void writeHeader(FILE* f) {
        int n_points = int(points.size());
        fprintf(f, "# .PCD v.7 - Point Cloud Data file format\n");
        fprintf(f, "VERSION .7\n");
        fprintf(f, "FIELDS x y z spacing decay\n");
        fprintf(f, "SIZE  4 4 4 4 4\n");
        fprintf(f, "TYPE  F F F F F\n");
        fprintf(f, "COUNT 1 1 1 1 1\n");
        fprintf(f, "WIDTH %i\n", n_points);
        fprintf(f, "HEIGHT 1\n");
        fprintf(f, "VIEWPOINT 0 0 0 1 0 0 0\n");
        fprintf(f, "POINTS %i\n", n_points);
        fprintf(f, "DATA ascii\n");
    }

    inline void writeBody(FILE* f) {
        int n_points = int(points.size());
        for (int i = 0; i < n_points; i++)
            fprintf(f, "%e %e %e %e %e\n", points[i][0], points[i][1], points[i][2], spacing[i], decay[i]);
    }

    inline void readBody(FILE* f) {
        int n_points = extractPointCount(f);
        rewind(f);
        skipLines(f, 11);
        for (int i = 0; i < n_points; i++) {
            Parfait::Point<double> p;
            double s, d;
            fscanf(f, "%lf %lf %lf %lf %lf\n", &p[0], &p[1], &p[2], &s, &d);
            points.push_back(p);
            spacing.push_back(s);
            decay.push_back(d);
        }
    }

    inline int extractPointCount(FILE* f) {
        using namespace Parfait::StringTools;
        rewind(f);
        skipLines(f, 9);
        std::string line;
        Parfait::FileTools::readNextLine(f, line);
        auto words = split(line, " ");
        if (words.size() != 2 or words.front() != "POINTS") {
            throw std::logic_error("Expected: POINTS <number>\nActual: " + line);
        }
        int n = toInt(strip(words.back()));
        return n;
    }

    inline void skipLines(FILE* f, int n) {
        std::string line;
        for (int i = 0; i < n; i++) Parfait::FileTools::readNextLine(f, line);
    }
};
}

#pragma once
#include <MessagePasser/MessagePasser.h>
#include <string>
#include <vector>
#include "Point.h"
#include <parfait/StringTools.h>

namespace Parfait {
class PointWriter {
  public:
    inline static void write(MessagePasser mp,
                             std::string filename,
                             const std::vector<Parfait::Point<double>>& points) {
        std::vector<double> field(points.size(), 1.0);
        write(mp, filename, points, field);
    }
    inline static void write(MessagePasser mp,
                             std::string filename,
                             const std::vector<Parfait::Point<double>>& points,
                             const std::vector<double>& field) {
        FILE* fp = nullptr;
        int root = 0;
        if (mp.Rank() == root) {
            fp = open(filename);
            writeHeader(fp);
        }

        size_t n_global_items = mp.ParallelSum(points.size());

        size_t megabyte = 1024 * 1024;
        size_t chunk_max_size_in_MB = 100;
        size_t chunk_size = chunk_max_size_in_MB * megabyte / 4 * sizeof(double);
        size_t num_written = 0;
        std::vector<Parfait::Point<double>> chunk_points;
        std::vector<double> chunk_field;
        while (num_written < n_global_items) {
            long num_to_write = std::min(chunk_size, n_global_items - num_written);
            long start = num_written;
            long end = start + num_to_write;
            chunk_points = mp.Gather(points, start, end);
            chunk_field = mp.Gather(field, start, end);
            if (mp.Rank() == root) writePoints(fp, chunk_points, chunk_field);
            num_written += num_to_write;
        }
        if (mp.Rank() == 0) fclose(fp);
    }
    inline static void write(std::string filename,
                             const std::vector<Parfait::Point<double>>& points,
                             const std::vector<double>& field) {
        FILE* fp = open(filename);
        writeHeader(fp);
        writePoints(fp, points, field);
        fclose(fp);
    }
    inline static void write(std::string filename, const std::vector<Parfait::Point<double>>& points) {
        std::vector<double> field(points.size(), 1.0);
        write(filename, points, field);
    }
    inline static void write(std::string filename,
                             const std::vector<Parfait::Point<double>>& points,
                             const std::vector<int>& field) {
        std::vector<double> double_field(points.size());
        for (size_t i = 0; i < double_field.size(); i++) double_field[i] = field[i];
        write(filename, points, double_field);
    }

  private:
    inline static FILE* open(std::string& filename) {
        auto split = StringTools::split(filename, ".");
        if (split.back() != "3D") {
            filename += ".3D";
        }
        FILE* fp = fopen(filename.c_str(), "w");
        return fp;
    }
    inline static void writeHeader(FILE* fp) { fprintf(fp, "x y z field\n"); }
    inline static void writePoints(FILE* fp,
                                   const std::vector<Parfait::Point<double>>& points,
                                   const std::vector<double>& field) {
        for (size_t i = 0; i < points.size(); i++) {
            auto& p = points[i];
            fprintf(fp, "%1.15e %1.15e %1.15e %1.15e\n", p[0], p[1], p[2], field[i]);
        }
    }
};
}

#pragma once

#include <parfait/Facet.h>
#include <parfait/STLFactory.h>
#include <parfait/FileTools.h>
#include <parfait/StringTools.h>

namespace Parfait {
struct TriMesh {
  public:
    std::vector<Parfait::Point<double>> points;
    std::vector<std::array<int, 3>> triangles;
    std::vector<int> tags;
};
class TriWriter {
  public:
    inline static void write(std::string filename,
                             const std::vector<Parfait::Point<double>>& points,
                             const std::vector<std::array<int, 3>>& tris,
                             const std::vector<int>& tags) {
        FILE* fp = fopen(filename.c_str(), "w");
        if (fp == nullptr) {
            PARFAIT_THROW("Could not open file for reading: " + filename);
        }
        int num_points = int(points.size());
        int num_tris = int(tris.size());
        int num_tags = int(tags.size());
        PARFAIT_ASSERT(num_tris == num_tags, "Number of tags doesn't match number of triangles");

        fprintf(fp, "%d %d\n", num_points, num_tris);
        for (int n = 0; n < num_points; n++) {
            fprintf(fp, "%e %e %e\n", points[n][0], points[n][1], points[n][2]);
        }

        int fortran = 1;
        for (int t = 0; t < num_tris; t++) {
            fprintf(fp, "%d %d %d\n", tris[t][0] + fortran, tris[t][1] + fortran, tris[t][2] + fortran);
        }

        for (int t = 0; t < num_tris; t++) {
            fprintf(fp, "%d\n", tags[t]);
        }

        fclose(fp);
    }
    inline static void write(std::string filename, const std::vector<Parfait::Facet>& facets) {
        FILE* fp = fopen(filename.c_str(), "w");
        if (fp == nullptr) {
            PARFAIT_THROW("Could not open file for reading: " + filename);
        }

        int num_nodes = int(facets.size()) * 3;
        int num_tris = int(facets.size());
        fprintf(fp, "%d %d\n", num_nodes, num_tris);
        std::vector<Parfait::Point<double>> points(num_nodes);
        for (int t = 0; t < num_tris; t++) {
            for (int i = 0; i < 3; i++) {
                fprintf(fp, "%le %le %le\n", facets[t][i][0], facets[t][i][1], facets[t][i][2]);
            }
        }

        int fortran = 1;
        for (int t = 0; t < num_tris; t++) {
            int p1 = 3 * t + 0 + fortran;
            int p2 = 3 * t + 1 + fortran;
            int p3 = 3 * t + 2 + fortran;
            fprintf(fp, "%d %d %d\n", p1, p2, p3);
        }
        for (int t = 0; t < num_tris; t++) {
            fprintf(fp, "1\n");
        }
        fclose(fp);
    }
};

class TriReader {
  public:
    inline static TriMesh read(std::string filename) {
        FILE* fp = fopen(filename.c_str(), "r");
        if (fp == nullptr) {
            PARFAIT_THROW("Could not open file for reading: " + filename);
        }

        int num_nodes;
        int num_tris;
        fscanf(fp, "%d %d\n", &num_nodes, &num_tris);

        TriMesh mesh;
        mesh.points.resize(num_nodes);
        mesh.triangles.resize(num_tris);
        mesh.tags.resize(num_tris);

        for (auto& p : mesh.points) {
            fscanf(fp, "%le %le %le\n", &p[0], &p[1], &p[2]);
        }

        for (auto& t : mesh.triangles) {
            fscanf(fp, "%d %d %d\n", &t[0], &t[1], &t[2]);
            t[0]--;  // fortran
            t[1]--;  // fortran
            t[2]--;  // fortran
        }

        for (auto& t : mesh.tags) {
            fscanf(fp, "%d\n", &t);
        }

        fclose(fp);
        return mesh;
    }
    inline static std::vector<Parfait::Facet> readFacets(std::string filename) {
        FILE* fp = fopen(filename.c_str(), "r");
        if (fp == nullptr) {
            PARFAIT_THROW("Could not open file for reading: " + filename);
        }

        int num_nodes;
        int num_tris;
        fscanf(fp, "%d %d\n", &num_nodes, &num_tris);
        std::vector<Parfait::Point<double>> points(num_nodes);
        for (int i = 0; i < num_nodes; i++) {
            fscanf(fp, "%le %le %le\n", &points[i][0], &points[i][1], &points[i][2]);
        }

        std::vector<Parfait::Facet> facets(num_tris);
        for (int i = 0; i < num_tris; i++) {
            int p1, p2, p3;
            fscanf(fp, "%d %d %d\n", &p1, &p2, &p3);
            p1--;  // fortran
            p2--;  // fortran
            p3--;  // fortran
            facets[i][0] = points[p1];
            facets[i][1] = points[p2];
            facets[i][2] = points[p3];
        }
        fclose(fp);
        return facets;
    }
};
}

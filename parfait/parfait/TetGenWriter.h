#pragma once
#include "StringTools.h"
#include "VizualizationWriter.h"

namespace Parfait {
namespace TetGen {
    template <typename GetPoint, typename FillTri, typename GetTriTag, typename FillQuad, typename GetQuadTag>
    void writeSmesh(std::string filename,
                    GetPoint getPoint,
                    int num_points,
                    FillTri fillTri,
                    GetTriTag getTriTag,
                    int num_tris,
                    FillQuad fillQuad,
                    GetQuadTag getQuadTag,
                    int num_quads) {
        auto extension = Parfait::StringTools::getExtension(filename);
        if (extension != "smesh") {
            filename += ".smesh";
        }
        FILE* fp = fopen(filename.c_str(), "w");
        PARFAIT_ASSERT(fp != nullptr, "Could not open file for writing " + filename);

        int dimension = 3;
        int no_attribute = 0;
        int no_boundary_marker = 0;
        fprintf(fp, "%d %d %d %d\n", num_points, dimension, no_attribute, no_boundary_marker);
        for (int i = 0; i < num_points; i++) {
            auto p = getPoint(i);
            fprintf(fp, "%d %e %e %e\n", i + 1, p[0], p[1], p[2]);
        }

        int num_elements = num_tris + num_quads;

        int one_tag = 1;
        fprintf(fp, "%d %d\n", num_elements, one_tag);
        {
            std::array<long, 3> e;
            for (int i = 0; i < num_tris; i++) {
                fillTri(i, e.data());
                int tag = getTriTag(i);
                fprintf(fp, "3 %ld %ld %ld %d\n", e[0] + 1, e[1] + 1, e[2] + 1, tag);
            }
        }

        {
            std::array<long, 4> e;
            for (int i = 0; i < num_quads; i++) {
                fillQuad(i, e.data());
                int tag = getQuadTag(i);
                fprintf(fp, "4 %ld %ld %ld %ld %d\n", e[0] + 1, e[1] + 1, e[2] + 1, e[3] + 1, tag);
            }
        }

        fclose(fp);
    }

    template <typename GetPoint, typename FillTri, typename GetTriTag, typename FillQuad, typename GetQuadTag>
    void writePoly(std::string filename,
                   GetPoint getPoint,
                   int num_points,
                   FillTri fillTri,
                   GetTriTag getTriTag,
                   int num_tris,
                   FillQuad fillQuad,
                   GetQuadTag getQuadTag,
                   int num_quads,
                   int num_hole_points = 0,
                   std::function<Parfait::Point<double>(int)> get_hole_point = nullptr) {
        auto ext = Parfait::StringTools::getExtension(filename);
        if (ext != "poly") filename += ".poly";
        FILE* fp = fopen(filename.c_str(), "w");
        int dimension = 3;
        int no_attribute = 0;
        int no_boundary_marker = 0;
        fprintf(fp, "%d %d %d %d\n", num_points, dimension, no_attribute, no_boundary_marker);
        for (int i = 0; i < num_points; i++) {
            auto p = getPoint(i);
            fprintf(fp, "%d %1.15e %1.15e %1.15e\n", i + 1, p[0], p[1], p[2]);
        }
        int face_count = num_tris + num_quads;
        no_boundary_marker = 0;
        int no_holes = 0;
        fprintf(fp, "%d %d %d\n", face_count, no_holes, no_boundary_marker);
        std::array<long, 3> t;
        std::array<long, 4> q;
        for (int i = 0; i < num_tris; i++) {
            fillTri(i, t.data());
            fprintf(fp, "1 0 0\n3 %ld %ld %ld\n", t[0] + 1, t[1] + 1, t[2] + 1);
        }
        for (int i = 0; i < num_quads; i++) {
            fillQuad(i, q.data());
            fprintf(fp, "1 0 0\n4 %ld %ld %ld %ld\n", q[0] + 1, q[1] + 1, q[2] + 1, q[3] + 1);
        }
        fprintf(fp, "%d\n", num_hole_points);
        for (int i = 0; i < num_hole_points; i++) {
            fprintf(fp, "%d, %s\n", i, get_hole_point(i).to_string().c_str());
        }
        int no_region = 0;
        fprintf(fp, "%d\n", no_region);
        fclose(fp);
    }
}
}

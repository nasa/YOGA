#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/Point.h>
#include <vector>

class ScatterPlotter {
  public:
    static void plot(const std::string filename,
                     MessagePasser mp,
                     std::function<long()> get_n_pts,
                     std::function<void(int, double*)> get_xyz,
                     std::function<void(int, long*)> get_gid,
                     std::function<void(int, double*)> get_scalar) {
        auto pts = collapse(mp, get_n_pts, get_xyz);
        auto gids = collapse<long>(mp, get_n_pts, get_gid);
        auto scalars = collapse<double>(mp, get_n_pts, get_scalar);

        if (mp.Rank() == 0) {
            FILE* f = fopen(filename.c_str(), "w");
            int n = pts.size();
            fprintf(f, "x coord, y coord, z coord, global id, scalar\n");
            for (int i = 0; i < n; i++) {
                auto& p = pts[i];
                fprintf(f, "%lf, %lf, %lf, %li, %lf\n", p[0], p[1], p[2], gids[i], scalars[i]);
            }
            fclose(f);
        }
    }
    static std::vector<Parfait::Point<double>> collapse(MessagePasser mp,
                                                        std::function<long()> get_n_pts,
                                                        std::function<void(int, double*)> get_xyz) {
        int n = get_n_pts();
        std::vector<Parfait::Point<double>> pts(n);
        for (int i = 0; i < n; i++) {
            get_xyz(i, pts[i].data());
        }
        auto vecs = mp.Gather(pts);
        std::vector<Parfait::Point<double>> all_points;
        for (auto& v : vecs) {
            all_points.insert(all_points.end(), v.begin(), v.end());
        }
        return all_points;
    }

    template <typename T>
    static std::vector<T> collapse(MessagePasser mp,
                                   std::function<long()> get_n_pts,
                                   std::function<void(int, T*)> get_gid) {
        int n = get_n_pts();
        std::vector<T> gids(n);
        for (int i = 0; i < n; i++) {
            get_gid(i, &gids[i]);
        }
        auto vecs = mp.Gather(gids);
        std::vector<T> all_gids;
        for (auto& v : vecs) {
            all_gids.insert(all_gids.end(), v.begin(), v.end());
        }
        return all_gids;
    }
};

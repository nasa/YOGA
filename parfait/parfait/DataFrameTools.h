#pragma once
#include "Point.h"
#include "DataFrame.h"
#include "StringTools.h"
#include "ToString.h"

namespace Parfait {
namespace DataFrameTools {
    inline std::string findUpperOrLower(const Parfait::DataFrame& df, std::string lowercase) {
        auto key = lowercase;
        if (df.has(key)) return key;
        key = Parfait::StringTools::toupper(key);
        if (df.has(key)) return key;
        return "";
    }
    inline std::vector<Parfait::Point<double>> extractPoints(const Parfait::DataFrame& df) {
        std::string x = findUpperOrLower(df, "x");
        std::string y = findUpperOrLower(df, "y");
        std::string z = findUpperOrLower(df, "z");
        if (x.empty() or y.empty() or z.empty()) {
            PARFAIT_THROW("Could not parse dataframe as x,y,z point cloud.  Column names: " +
                          Parfait::to_string(df.columns()));
        }

        int num_points = int(df[x].size());
        std::vector<Parfait::Point<double>> xyz(num_points);
        auto& x_vec = df[x];
        auto& y_vec = df[y];
        auto& z_vec = df[z];
        for (int n = 0; n < num_points; n++) {
            xyz[n] = {x_vec[n], y_vec[n], z_vec[n]};
        }
        return xyz;
    }

}
}
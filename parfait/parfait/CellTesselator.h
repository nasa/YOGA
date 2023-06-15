#pragma once
#include <parfait/Point.h>
#include <array>

namespace Parfait {

class CellTesselator {
  public:
    typedef std::array<Parfait::Point<double>, 4> Tet;
    typedef std::array<Parfait::Point<double>, 5> Pyramid;
    typedef std::array<Parfait::Point<double>, 6> Prism;
    typedef std::array<Parfait::Point<double>, 8> Hex;

    static std::array<Tet, 1> tesselate(const Tet& tet) { return {tet}; };
    static std::array<Tet, 1> tesselateNonConformally(const Tet& tet) { return tesselate(tet); };

    static std::array<Tet, 4> tesselate(const Pyramid& pyramid) {
        std::array<Tet, 4> tets;
        auto base_centroid = (pyramid[0] + pyramid[1] + pyramid[2] + pyramid[3]) * 0.25;
        tets[0] = {pyramid[0], pyramid[4], pyramid[1], base_centroid};
        tets[1] = {pyramid[1], pyramid[4], pyramid[2], base_centroid};
        tets[2] = {pyramid[2], pyramid[4], pyramid[3], base_centroid};
        tets[3] = {pyramid[3], pyramid[4], pyramid[0], base_centroid};
        return tets;
    }

    static std::array<Tet, 4> tesselateNonConformally(const Pyramid& pyramid) {
        std::array<Tet, 4> tets;
        // split along diagonal
        tets[0] = {pyramid[0], pyramid[1], pyramid[2], pyramid[4]};
        tets[1] = {pyramid[0], pyramid[2], pyramid[3], pyramid[4]};
        // split along other diagonal
        tets[2] = {pyramid[1], pyramid[2], pyramid[3], pyramid[4]};
        tets[3] = {pyramid[1], pyramid[3], pyramid[0], pyramid[4]};
        return tets;
    }

    static std::array<Tet, 14> tesselate(const Prism& prism) {
        std::array<Tet, 14> tets;

        auto cell_centroid = calcCentroid<6>(prism);

        auto face_centroid = calcCentroid<4>({prism[0], prism[1], prism[4], prism[3]});
        tets[0] = {prism[0], face_centroid, prism[1], cell_centroid};
        tets[1] = {prism[1], face_centroid, prism[4], cell_centroid};
        tets[2] = {prism[3], face_centroid, prism[0], cell_centroid};
        tets[3] = {prism[4], face_centroid, prism[3], cell_centroid};

        face_centroid = calcCentroid<4>({prism[1], prism[2], prism[5], prism[4]});
        tets[4] = {prism[1], face_centroid, prism[2], cell_centroid};
        tets[5] = {prism[2], face_centroid, prism[5], cell_centroid};
        tets[6] = {prism[4], face_centroid, prism[1], cell_centroid};
        tets[7] = {prism[5], face_centroid, prism[4], cell_centroid};

        face_centroid = calcCentroid<4>({prism[0], prism[2], prism[3], prism[5]});
        tets[8] = {prism[0], face_centroid, prism[3], cell_centroid};
        tets[9] = {prism[2], face_centroid, prism[0], cell_centroid};
        tets[10] = {prism[3], face_centroid, prism[5], cell_centroid};
        tets[11] = {prism[5], face_centroid, prism[2], cell_centroid};

        tets[12] = {prism[0], prism[1], prism[2], cell_centroid};
        tets[13] = {prism[3], prism[5], prism[4], cell_centroid};

        return tets;
    };

    static std::array<Tet, 14> tesselateNonConformally(const Prism& prism) {
        std::array<Tet, 14> tets;

        auto cell_centroid = calcCentroid<6>(prism);

        tets[0] = {prism[0], prism[1], prism[2], cell_centroid};
        tets[1] = {prism[5], prism[4], prism[3], cell_centroid};
        tets[2] = {prism[0], prism[3], prism[1], cell_centroid};
        tets[3] = {prism[1], prism[3], prism[4], cell_centroid};
        tets[4] = {prism[1], prism[4], prism[2], cell_centroid};
        tets[5] = {prism[2], prism[4], prism[5], cell_centroid};
        tets[6] = {prism[2], prism[5], prism[3], cell_centroid};
        tets[7] = {prism[2], prism[3], prism[0], cell_centroid};
        tets[8] = {prism[3], prism[4], prism[0], cell_centroid};
        tets[9] = {prism[0], prism[4], prism[1], cell_centroid};
        tets[10] = {prism[1], prism[5], prism[2], cell_centroid};
        tets[11] = {prism[5], prism[2], prism[4], cell_centroid};
        tets[12] = {prism[2], prism[5], prism[0], cell_centroid};
        tets[13] = {prism[0], prism[5], prism[3], cell_centroid};
        return tets;
    }

    static std::array<Tet, 24> tesselate(const Hex& hex) {
        std::array<Tet, 24> tets;

        auto cell_centroid = calcCentroid<8>(hex);

        auto face_centroid = calcCentroid<4>({hex[0], hex[1], hex[4], hex[5]});
        tets[0] = {hex[0], face_centroid, hex[1], cell_centroid};
        tets[1] = {hex[1], face_centroid, hex[5], cell_centroid};
        tets[2] = {hex[5], face_centroid, hex[4], cell_centroid};
        tets[3] = {hex[4], face_centroid, hex[0], cell_centroid};

        face_centroid = calcCentroid<4>({hex[1], hex[2], hex[5], hex[6]});
        tets[4] = {hex[1], face_centroid, hex[2], cell_centroid};
        tets[5] = {hex[2], face_centroid, hex[6], cell_centroid};
        tets[6] = {hex[6], face_centroid, hex[5], cell_centroid};
        tets[7] = {hex[5], face_centroid, hex[1], cell_centroid};

        face_centroid = calcCentroid<4>({hex[2], hex[3], hex[6], hex[7]});
        tets[8] = {hex[2], face_centroid, hex[3], cell_centroid};
        tets[9] = {hex[3], face_centroid, hex[7], cell_centroid};
        tets[10] = {hex[7], face_centroid, hex[6], cell_centroid};
        tets[11] = {hex[6], face_centroid, hex[2], cell_centroid};

        face_centroid = calcCentroid<4>({hex[3], hex[0], hex[4], hex[7]});
        tets[12] = {hex[3], face_centroid, hex[0], cell_centroid};
        tets[13] = {hex[0], face_centroid, hex[4], cell_centroid};
        tets[14] = {hex[4], face_centroid, hex[7], cell_centroid};
        tets[15] = {hex[7], face_centroid, hex[3], cell_centroid};

        face_centroid = calcCentroid<4>({hex[3], hex[2], hex[0], hex[1]});
        tets[16] = {hex[3], face_centroid, hex[2], cell_centroid};
        tets[17] = {hex[2], face_centroid, hex[1], cell_centroid};
        tets[18] = {hex[1], face_centroid, hex[0], cell_centroid};
        tets[19] = {hex[0], face_centroid, hex[3], cell_centroid};

        face_centroid = calcCentroid<4>({hex[4], hex[5], hex[6], hex[7]});
        tets[20] = {hex[4], face_centroid, hex[5], cell_centroid};
        tets[21] = {hex[5], face_centroid, hex[6], cell_centroid};
        tets[22] = {hex[6], face_centroid, hex[7], cell_centroid};
        tets[23] = {hex[7], face_centroid, hex[4], cell_centroid};

        return tets;
    };

    static std::array<Tet, 24> tesselateNonConformally(const Hex& hex) {
        std::array<Tet, 24> tets;

        auto cell_centroid = calcCentroid<8>(hex);
        // front quad
        tets[0] = {hex[0], hex[4], hex[1], cell_centroid};
        tets[1] = {hex[0], hex[5], hex[1], cell_centroid};
        tets[2] = {hex[0], hex[4], hex[5], cell_centroid};
        tets[3] = {hex[5], hex[1], hex[4], cell_centroid};
        // right quad
        tets[4] = {hex[1], hex[5], hex[2], cell_centroid};
        tets[5] = {hex[1], hex[5], hex[6], cell_centroid};
        tets[6] = {hex[1], hex[6], hex[2], cell_centroid};
        tets[7] = {hex[6], hex[2], hex[5], cell_centroid};
        // back quad
        tets[8] = {hex[2], hex[6], hex[3], cell_centroid};
        tets[9] = {hex[2], hex[7], hex[3], cell_centroid};
        tets[10] = {hex[2], hex[6], hex[7], cell_centroid};
        tets[11] = {hex[7], hex[3], hex[6], cell_centroid};
        // left quad
        tets[12] = {hex[3], hex[7], hex[4], cell_centroid};
        tets[13] = {hex[3], hex[7], hex[0], cell_centroid};
        tets[14] = {hex[3], hex[4], hex[0], cell_centroid};
        tets[15] = {hex[4], hex[0], hex[7], cell_centroid};
        // bottom quad
        tets[16] = {hex[0], hex[1], hex[3], cell_centroid};
        tets[17] = {hex[0], hex[1], hex[2], cell_centroid};
        tets[18] = {hex[0], hex[2], hex[3], cell_centroid};
        tets[19] = {hex[2], hex[3], hex[1], cell_centroid};
        // top quad
        tets[20] = {hex[4], hex[7], hex[5], cell_centroid};
        tets[21] = {hex[4], hex[7], hex[6], cell_centroid};
        tets[22] = {hex[4], hex[6], hex[5], cell_centroid};
        tets[23] = {hex[6], hex[5], hex[7], cell_centroid};
        return tets;
    }

  private:
    template <int N>
    static Parfait::Point<double> calcCentroid(const std::array<Parfait::Point<double>, N>& points) {
        Parfait::Point<double> c{0, 0, 0};
        for (auto& p : points) c += p;
        c *= 1.0 / double(N);
        return c;
    }
};

}
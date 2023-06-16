#pragma once
#include <algorithm>
namespace Parfait {

template <typename CellContainer>
void rewindLeftHandedTriangle(CellContainer& triangle) {
    std::swap(triangle[0], triangle[1]);
}

template <typename CellContainer>
void rewindLeftHandedQuad(CellContainer& quad) {
    std::swap(quad[0], quad[1]);
    std::swap(quad[2], quad[3]);
}

template <typename CellContainer>
void rewindLeftHandedTet(CellContainer& tet) {
    std::swap(tet[0], tet[1]);
}

template <typename CellContainer>
void rewindLeftHandedPyramid(CellContainer& pyramid) {
    std::swap(pyramid[0], pyramid[1]);
    std::swap(pyramid[2], pyramid[3]);
}

template <typename CellContainer>
void rewindLeftHandedPrism(CellContainer& prism) {
    std::swap(prism[0], prism[1]);
    std::swap(prism[3], prism[4]);
}

template <typename CellContainer>
void rewindLeftHandedHex(CellContainer& hex) {
    std::swap(hex[0], hex[1]);
    std::swap(hex[2], hex[3]);
    std::swap(hex[4], hex[5]);
    std::swap(hex[6], hex[7]);
}

}
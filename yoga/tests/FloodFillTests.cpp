#include <parfait/CartBlock.h>
#include <algorithm>
#include "CartBlockFloodFill.h"
#include <RingAssertions.h>

using namespace Parfait;
using namespace YOGA;

TEST_CASE("to flood fill a cartesian grid") {
    CartBlock block({0, 0, 0}, {1, 1, 1}, 5, 5, 5);

    SECTION("can identify neighbors") {
        std::vector<int> nbrs = CartBlockFloodFill::getNeighbors(0, block);
        REQUIRE(3 == nbrs.size());
        std::sort(nbrs.begin(), nbrs.end());
        REQUIRE(1 == nbrs[0]);
        REQUIRE(5 == nbrs[1]);
        REQUIRE(25 == nbrs[2]);
    }

    SECTION("Can add all outer cells as seeds to flood fill") {
        std::vector<int> cellStatuses(5 * 5 * 5, CartBlockFloodFill::Status::Untouched);

        SECTION("Note: there are 150 faces on a 5x5x5 cube, but only 98 cells in the outer layer") {
            std::vector<int> cellIds = CartBlockFloodFill::identifyAndMarkOuterCells(block, cellStatuses);
            REQUIRE(98 == cellIds.size());
            for (int id : cellIds) REQUIRE(id < 125);
        }
        SECTION("Make sure to skip over marked cells") {
            for (int i = 0; i < 5; ++i) cellStatuses[i] = CartBlockFloodFill::Status::Crossing;
            std::vector<int> cellIds = CartBlockFloodFill::identifyAndMarkOuterCells(block, cellStatuses);
            REQUIRE(93 == cellIds.size());
        }
        SECTION("Flood fill an empty block") {
            auto seeds = CartBlockFloodFill::identifyAndMarkOuterCells(block, cellStatuses);
            CartBlockFloodFill::stackFill(seeds, block, cellStatuses);
            int n = 0;
            for (auto s : cellStatuses)
                if (CartBlockFloodFill::Status::OutOfHole == s) ++n;
            REQUIRE(125 == n);
        }
        SECTION("Flood fill with a cube shaped hole in the block") {
            for (int i = 2; i < 5; ++i)
                for (int j = 2; j < 5; ++j)
                    for (int k = 2; k < 5; ++k)
                        cellStatuses[block.convert_ijk_ToCellId(i, j, k)] = CartBlockFloodFill::Status::Crossing;
            cellStatuses[block.convert_ijk_ToCellId(3, 3, 3)] = CartBlockFloodFill::Status::Untouched;
            auto seeds = CartBlockFloodFill::identifyAndMarkOuterCells(block, cellStatuses);
            CartBlockFloodFill::stackFill(seeds, block, cellStatuses);
            int n = 0;
            for (auto s : cellStatuses)
                if (CartBlockFloodFill::Status::OutOfHole == s) ++n;
            REQUIRE(98 == n);
            REQUIRE(CartBlockFloodFill::Status::InHole == cellStatuses[block.convert_ijk_ToCellId(3, 3, 3)]);
        }
    }
}

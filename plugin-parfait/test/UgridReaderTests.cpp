#include <map>
#include <string>
#include <vector>

#include "../shared/UgridReader.h"
#include <RingAssertions.h>
using namespace inf;

std::string six_cell = std::string(GRIDS_DIR) + "lb8.ugrid/6cell.lb8.ugrid";

TEST_CASE("Ugrid Reader can give a node count", "[only]") {
    UgridReader reader(six_cell);
    long node_count = reader.nodeCount();
    REQUIRE(node_count == 14);
}

TEST_CASE("Ugrid Reader can get the right number of cells") {
    UgridReader reader(six_cell);
    REQUIRE(0 == reader.cellCount(MeshInterface::TETRA_4));
    REQUIRE(12 == reader.cellCount(MeshInterface::TRI_3));
    REQUIRE(6 == reader.cellCount(MeshInterface::QUAD_4));
    REQUIRE(6 == reader.cellCount(MeshInterface::PENTA_6));
    REQUIRE(0 == reader.cellCount(MeshInterface::PYRA_5));
    REQUIRE(0 == reader.cellCount(MeshInterface::HEXA_8));
}

TEST_CASE("Ugrid reader can read nodes by slice") {
    UgridReader reader(six_cell);
    auto points = reader.readCoords(0, 1);
    REQUIRE(points.size() == 1);
}

TEST_CASE("Ugrid reader can read all nodes") {
    UgridReader reader(six_cell);
    auto points = reader.readCoords();
    REQUIRE(points.size() == 14);
}

TEST_CASE("Ugrid Reader can read prism elements") {
    UgridReader reader(six_cell);
    auto pyramids = reader.readCells(MeshInterface::PENTA_6, 0, 6);
    REQUIRE(6 * 6 == pyramids.size());
}

TEST_CASE("Ugrid Reader can read triangle elements") {
    UgridReader reader(six_cell);
    auto cells = reader.readCells(MeshInterface::TRI_3, 0, 6);
    REQUIRE(6 * 3 == cells.size());
    cells = reader.readCells(MeshInterface::QUAD_4, 0, 6);
    REQUIRE(4 * 6 == cells.size());
}

TEST_CASE("Ugrid Reader can read element tags") {
    UgridReader reader(six_cell);
    auto tags = reader.readCellTags(MeshInterface::PENTA_6, 0, 6);
    REQUIRE(tags.size() == 6);
    REQUIRE(tags[3] == UgridReader::NILL_TAG);
    tags = reader.readCellTags(MeshInterface::QUAD_4, 0, 1);
    REQUIRE(tags.size() == 1);
    REQUIRE(tags[0] == 0x7fffffff);
    // why is the 6 cell tag so big!  But that _is_ what is in the file,
    // verified with vtk-surface output
    // I'm guessing it got borked during a big-endian conversion at some point
}

TEST_CASE("Ugrid reader cell types") {
    UgridReader reader(six_cell);
    auto types = reader.cellTypes();
    REQUIRE(types.size() == 3);
}

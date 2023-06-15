#include <t-infinity/MeshInterface.h>
#include <RingAssertions.h>
#include <CGNSReader.h>

std::string sphere_prism_file = "../../../grids/cgns/sphere_prism_P1.cgns";

TEST_CASE("CGNS Reader can give node count") {
    CGNSReader reader(sphere_prism_file);
    long node_count = reader.nodeCount();
    REQUIRE(node_count == 7176);
}

TEST_CASE("CGNS Reader can get the right number of cells") {
    CGNSReader reader(sphere_prism_file);
    long count = reader.cellCount();
    REQUIRE(26672 == count);
    count -= reader.cellCount(inf::MeshInterface::CellType::TETRA_4);
    count -= reader.cellCount(inf::MeshInterface::CellType::PENTA_6);
    REQUIRE(0 == count);
}

TEST_CASE("CGNS Reader can give points in range") {
    CGNSReader reader(sphere_prism_file);
    auto points = reader.readCoords(0, 1);
    REQUIRE(points.size() == 1);
}
TEST_CASE("CGNS Reader reads an empty slice") {
    CGNSReader reader(sphere_prism_file);
    auto points = reader.readCoords(1, 1);
    REQUIRE(points.size() == 0);
}

TEST_CASE("give cell count for each element type") {
    CGNSReader reader(sphere_prism_file);
    long count = reader.cellCount(inf::MeshInterface::CellType::TETRA_4);
    REQUIRE(count == 19792);
}

TEST_CASE("count triangles") {
    CGNSReader reader(sphere_prism_file);
    long count = reader.cellCount(inf::MeshInterface::CellType::TRI_3);
    REQUIRE(1544 == count);
}

TEST_CASE("count tets") {
    CGNSReader reader(sphere_prism_file);
    long count = reader.cellCount(inf::MeshInterface::CellType::TETRA_4);
    REQUIRE(19792 == count);
}

TEST_CASE("count prisms") {
    CGNSReader reader(sphere_prism_file);
    long count = reader.cellCount(inf::MeshInterface::CellType::PENTA_6);
    REQUIRE(6880 == count);
}

TEST_CASE("read tets") {
    CGNSReader reader(sphere_prism_file);
    auto tets = reader.readCells(inf::MeshInterface::CellType::TETRA_4, 0, 19792);
    REQUIRE(79168 == tets.size());
}

TEST_CASE("read tris") {
    CGNSReader reader(sphere_prism_file);
    auto tris = reader.readCells(inf::MeshInterface::CellType::TRI_3, 0, 1);
    REQUIRE(3 == tris.size());
}

TEST_CASE("Read empty slice") {
    CGNSReader reader(sphere_prism_file);
    auto tris = reader.readCells(inf::MeshInterface::CellType::TRI_3, 1, 1);
    REQUIRE(0 == tris.size());
}

TEST_CASE("read some triangles") {
    CGNSReader reader(sphere_prism_file);
    auto tris = reader.readCells(inf::MeshInterface::CellType::TRI_3, 100, 344);
    REQUIRE(244 * 3 == tris.size());
}

TEST_CASE("boundary tag of range of elements") {
    CGNSReader reader(sphere_prism_file);
    auto tags = reader.readCellTags(inf::MeshInterface::CellType::TRI_3, 0, 400);
    REQUIRE(tags.size() == 400);
    REQUIRE(1 == tags[344]);  // the start of the second boundary.
}

TEST_CASE("cell tags of volume elements") {
    CGNSReader reader(sphere_prism_file);
    auto tags = reader.readCellTags(inf::MeshInterface::CellType::TETRA_4, 0, 400);
    REQUIRE(tags.size() == 400);
    REQUIRE(CGNSReader::NILL_TAG == tags[344]);
}

TEST_CASE("cell tags empty slice") {
    CGNSReader reader(sphere_prism_file);
    auto tags = reader.readCellTags(inf::MeshInterface::CellType::TETRA_4, 400, 400);
    REQUIRE(tags.size() == 0);
}

TEST_CASE("cell type list") {
    CGNSReader reader(sphere_prism_file);
    std::vector<inf::MeshInterface::CellType> types = reader.cellTypes();
    REQUIRE(types.size() == 3);
}

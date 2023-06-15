#include <RingAssertions.h>
#include <parfait/TriReader.h>

TEST_CASE("Can read ascii .tri file in Cart 3D format") {
    auto hash = Parfait::StringTools::randomLetters(5);
    auto filename = hash + ".tri";
    auto cube = Parfait::STLFactory::getUnitCube();
    Parfait::TriWriter::write(filename, cube);

    auto cube_read = Parfait::TriReader::readFacets(filename);
    REQUIRE(cube.size() == cube_read.size());

    auto cube_mesh = Parfait::TriReader::read(filename);
    REQUIRE(cube_mesh.triangles.size() == cube.size());

    int lowest_node = std::numeric_limits<int>::max();

    for (auto& t : cube_mesh.triangles) {
        for (auto n : t) {
            lowest_node = std::min(lowest_node, n);
        }
    }
    REQUIRE(lowest_node == 0);  // require we convert from fortran
}
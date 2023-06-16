#include <RingAssertions.h>
#include <parfait/ObjReader.h>
#include <parfait/STLFactory.h>

TEST_CASE("Can read a basic obj file", "[obj]") {
    auto sphere = Parfait::STLFactory::getUnitCube();
    auto hash = Parfait::StringTools::randomLetters(5);
    Parfait::ObjWriter::write(hash + ".obj", sphere);

    auto sphere_read = Parfait::ObjReader::read(hash + ".obj");
    REQUIRE(sphere_read.size() == sphere.size());
}

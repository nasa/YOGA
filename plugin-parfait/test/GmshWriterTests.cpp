#include <RingAssertions.h>
#include <SerialMeshImporter.h>
#include <GmshReader.h>
#include "../shared/MockReaders.h"
#include <SerialGmshExporter.h>

TEST_CASE("Given a serial mesh, export it to file") {
    TwoTetReader reader;
    auto mesh = SerialMeshImporter::import(reader);
    SerialGmshExporter::writeFile("test.msh", *mesh);

    try {
        GmshReader gmsh_reader("test.msh");
        auto mesh2 = SerialMeshImporter::import(gmsh_reader);
    } catch (const std::exception& e) {
        printf("Caught: %s\n", e.what());
    }
}

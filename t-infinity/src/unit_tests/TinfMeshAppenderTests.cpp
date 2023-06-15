#include <RingAssertions.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/TinfMeshAppender.h>

TEST_CASE("TinfMeshAppender can export an empty mesh") {
    TinfMeshAppender appender;
    inf::TinfMeshData data = appender.getData();
    REQUIRE(data.cells.size() == 0);
}
TEST_CASE("TinfMeshAdapter can add one cell") {
    long global_id = 901;
    int tag = 10;
    int owner = 0;
    std::vector<long> nodes = {100, 106, 105};
    std::vector<Parfait::Point<double>> points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    std::vector<int> node_owner = {1, 1, 2};
    inf::TinfMeshCell cell{inf::MeshInterface::TRI_3, global_id, tag, owner, nodes, points, node_owner};

    TinfMeshAppender appender;
    appender.addCell(cell);

    SECTION("can add initial cell") {
        auto data = appender.getData();
        REQUIRE(data.cells.size() == 1);
        REQUIRE(data.cells.at(inf::MeshInterface::TRI_3).size() == 3);
        REQUIRE(data.cell_tags.at(inf::MeshInterface::TRI_3).size() == 1);
        REQUIRE(data.cell_tags.at(inf::MeshInterface::TRI_3).at(0) == 10);
        REQUIRE(data.global_cell_id.at(inf::MeshInterface::TRI_3).at(0) == 901);
        REQUIRE(data.cell_owner.at(inf::MeshInterface::TRI_3).at(0) == 0);
        REQUIRE(data.points.size() == 3);
        REQUIRE(data.node_owner.size() == 3);
        REQUIRE(data.global_node_id.size() == 3);
    }

    nodes = {100, 106, 107};
    points = {{0, 0, 0}, {1, 0, 0}, {0, 0, 1}};
    node_owner = {1, 1, 7};
    global_id = 902;

    cell = inf::TinfMeshCell{inf::MeshInterface::TRI_3, global_id, tag, owner, nodes, points, node_owner};
    appender.addCell(cell);

    SECTION("won't add duplicate nodes") {
        auto data = appender.getData();
        REQUIRE(data.cells.size() == 1);
        REQUIRE(data.cells.at(inf::MeshInterface::TRI_3).size() == 6);
        REQUIRE(data.global_node_id.size() == 4);
        REQUIRE(data.cells.at(inf::MeshInterface::TRI_3).at(0) == 0);
    }

    SECTION("won't add duplicate cell") {
        appender.addCell(cell);
        auto data = appender.getData();
        REQUIRE(data.cells.size() == 1);
        REQUIRE(data.cells.at(inf::MeshInterface::TRI_3).size() == 6);
        REQUIRE(data.global_node_id.size() == 4);
        REQUIRE(data.cells.at(inf::MeshInterface::TRI_3).at(0) == 0);
    }
}

TEST_CASE("Can pack TinfMeshData") {
    long global_id = 901;
    int tag = 10;
    int owner = 0;
    std::vector<long> nodes = {100, 106, 105};
    std::vector<Parfait::Point<double>> points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    std::vector<int> node_owner = {1, 1, 2};
    inf::TinfMeshCell cell{inf::MeshInterface::TRI_3, global_id, tag, owner, nodes, points, node_owner};

    TinfMeshAppender appender;
    appender.addCell(cell);

    auto mesh = appender.getData();

    MessagePasser::Message message;
    mesh.pack(message);

    inf::TinfMeshData other_mesh;
    other_mesh.unpack(message);

    REQUIRE(mesh.node_owner == other_mesh.node_owner);
    REQUIRE(mesh.cells == other_mesh.cells);
    REQUIRE(mesh.cell_tags == other_mesh.cell_tags);
    REQUIRE(mesh.points == other_mesh.points);
    REQUIRE(mesh.cell_owner == other_mesh.cell_owner);
    REQUIRE(mesh.global_cell_id == other_mesh.global_cell_id);
    REQUIRE(mesh.global_node_id == other_mesh.global_node_id);
}

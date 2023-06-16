#include <t-infinity/Cell.h>
#include <t-infinity/Hiro.h>
#include <t-infinity/MeshInterface.h>
#include <parfait/Checkpoint.h>
#include <parfait/ToString.h>
#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include "MockMeshes.h"

TEST_CASE("Cell helper class can be constructed from a mesh and an id") {
    int mock_rank = 0;
    inf::TinfMesh mesh(inf::mock::onePrism(), mock_rank);
    inf::Cell cell(mesh, 0);

    REQUIRE(inf::MeshInterface::PENTA_6 == cell.type());
    auto nodes = cell.nodes();
    REQUIRE(nodes.size() == 6);
    REQUIRE(nodes[0] == 0);
    REQUIRE(nodes[1] == 1);
    REQUIRE(nodes[2] == 2);
    REQUIRE(nodes[3] == 3);
    REQUIRE(nodes[4] == 4);
    REQUIRE(nodes[5] == 5);

    int face_count = cell.faceCount();
    REQUIRE(face_count == 5);
    auto face_nodes = cell.faceNodes(0);
    REQUIRE(face_nodes.size() == 4);

    auto points = cell.points();
    REQUIRE(points.size() == 6);
    REQUIRE(points[0].approxEqual({0, 0, 0}));
    REQUIRE(points[1].approxEqual({1, 0, 0}));
    REQUIRE(points[2].approxEqual({0, 1, 0}));
    REQUIRE(points[3].approxEqual({0, 0, 1}));
    REQUIRE(points[4].approxEqual({1, 0, 1}));
    REQUIRE(points[5].approxEqual({0, 1, 1}));

    auto extent = cell.extent();
    REQUIRE(extent.lo.approxEqual({0, 0, 0}));
    REQUIRE(extent.hi.approxEqual({1, 1, 1}));
}

TEST_CASE("Cell helper class can be constructed from nodes and points") {
    int mock_rank = 0;
    inf::TinfMesh mesh(inf::mock::onePrism(), mock_rank);
    inf::Cell cell_mesh_constructor(mesh, 0);
    inf::Cell cell_vector_constructor(
        cell_mesh_constructor.type(), cell_mesh_constructor.nodes(), cell_mesh_constructor.points());
    inf::Cell cell_pointer_constructor(
        cell_mesh_constructor.type(), cell_mesh_constructor.nodes().data(), cell_mesh_constructor.points().data());
    auto cell = cell_pointer_constructor;

    REQUIRE(inf::MeshInterface::PENTA_6 == cell.type());
    auto nodes = cell.nodes();
    REQUIRE(nodes.size() == 6);
    REQUIRE(nodes[0] == 0);
    REQUIRE(nodes[1] == 1);
    REQUIRE(nodes[2] == 2);
    REQUIRE(nodes[3] == 3);
    REQUIRE(nodes[4] == 4);
    REQUIRE(nodes[5] == 5);

    int face_count = cell.faceCount();
    REQUIRE(face_count == 5);
    auto face_nodes = cell.faceNodes(0);
    REQUIRE(face_nodes.size() == 4);

    auto points = cell.points();
    REQUIRE(points.size() == 6);
}

Parfait::Point<double> calcGeometricCentroidOfTriangle(const std::vector<Parfait::Point<double>>& triangle) {
    Parfait::Point<double> edge_midpoint, geometric_centroid;
    edge_midpoint = 0.5 * (triangle[0] + triangle[1]);
    geometric_centroid = edge_midpoint;
    edge_midpoint = 0.5 * (triangle[1] + triangle[2]);
    geometric_centroid += edge_midpoint;
    edge_midpoint = 0.5 * (triangle[0] + triangle[2]);
    geometric_centroid += edge_midpoint;
    geometric_centroid /= 3.0;
    return geometric_centroid;
}
double calcSkewness(const Parfait::Point<double>& ck,
                    const Parfait::Point<double>& cj,
                    const Parfait::Point<double>& face_normal) {
    auto edge_vector = ck - cj;
    edge_vector.normalize();
    double skewness = Parfait::Point<double>::dot(edge_vector, face_normal);
    return skewness;
}

double calcSkewnessAsAspectRatioApproachesInfinity(Parfait::Point<double> a,
                                                   Parfait::Point<double> b,
                                                   Parfait::Point<double> c,
                                                   Parfait::Point<double> d,
                                                   double power) {
    double skewness = 1.0;
    for (int i = 0; i < 30; i++) {
        std::vector<Parfait::Point<double>> triangle = {a, b, c};
        std::vector<Parfait::Point<double>> top_neighbor = {a, c, d};

        auto top_centroid = inf::Hiro::calcCentroidFor2dElement(top_neighbor, power);
        auto bottom_centroid = inf::Hiro::calcCentroidFor2dElement(triangle, power);
        double dx = c[0] - a[0];
        double dy = c[1] - a[1];
        Parfait::Point<double> face_normal = {-dy, dx, 0};
        face_normal.normalize();
        skewness = calcSkewness(top_centroid, bottom_centroid, face_normal);

        d[1] *= 0.5;
        c[1] *= 0.5;
    }
    return skewness;
}

Parfait::Point<double> calcTriangleAreaVector(const std::vector<Parfait::Point<double>>& face) {
    auto& a = face[0];
    auto& b = face[1];
    auto& c = face[2];
    auto v1 = b - a;
    auto v2 = c - a;
    return Parfait::Point<double>::cross(v1, v2);
}

TEST_CASE("Hiro Face Centroid for a face of points") {
    Parfait::Point<double> a, b, c, d;
    a = {0, 0, 0};
    b = {1, 0, 0};
    c = {1, 1, 0};
    d = {0, 1, 0};

    SECTION("Verify that geometric centroid is recovered when p=0") {
        std::vector<int> triangle_node_ids{0, 1, 2};
        std::vector<Parfait::Point<double>> triangle_points{a, b, c};
        inf::Cell bottom_triangle(inf::MeshInterface::TRI_3, triangle_node_ids, triangle_points);
        auto geometric_centroid = calcGeometricCentroidOfTriangle(triangle_points);
        auto centroid = inf::Hiro::calcCentroid(bottom_triangle, 0);
        REQUIRE(centroid[0] == Approx(geometric_centroid[0]));
        REQUIRE(centroid[1] == Approx(geometric_centroid[1]));
        REQUIRE(centroid[2] == Approx(geometric_centroid[2]).margin(0.0001));
    }

    SECTION("p = 2") {
        std::vector<int> triangle_node_ids{0, 1, 2};
        std::vector<Parfait::Point<double>> triangle_points = {a, b, c};
        inf::Cell bottom_triangle(inf::MeshInterface::TRI_3, triangle_node_ids, triangle_points);
        auto centroid = inf::Hiro::calcCentroid(bottom_triangle, 2);
        REQUIRE(centroid[0] == Approx(0.625));
        REQUIRE(centroid[1] == Approx(0.375));
        REQUIRE(centroid[2] == Approx(0.0).margin(0.0001));
    }

    SECTION("calculate skewness of a face (of a triangle) with a given centroid for p=0,1,2") {
        double power = 0.0;
        double skewness = calcSkewnessAsAspectRatioApproachesInfinity(a, b, c, d, power);
        REQUIRE(0.0 == Approx(skewness).margin(1.0e-6));
        power = 1.0;
        skewness = calcSkewnessAsAspectRatioApproachesInfinity(a, b, c, d, power);
        REQUIRE((0.5 * std::sqrt(2.0)) == Approx(skewness).margin(1.0e-6));
        power = 2.0;
        skewness = calcSkewnessAsAspectRatioApproachesInfinity(a, b, c, d, power);
        REQUIRE(1.0 == Approx(skewness).margin(1.0e-6));
    }
}

Parfait::Point<double> calcTetGeometricCentroid(const std::vector<Parfait::Point<double>>& tet) {
    Parfait::Point<double> a, b, c, d;
    a = tet[0];
    b = tet[1];
    c = tet[2];
    d = tet[3];

    std::vector<Parfait::Point<double>> face_1, face_2, face_3, face_4;
    face_1 = {a, c, b};
    face_2 = {a, b, d};
    face_3 = {b, c, d};
    face_4 = {c, a, d};

    auto centroid = calcGeometricCentroidOfTriangle(face_1);
    centroid += calcGeometricCentroidOfTriangle(face_2);
    centroid += calcGeometricCentroidOfTriangle(face_3);
    centroid += calcGeometricCentroidOfTriangle(face_4);
    centroid /= 4.0;
    return centroid;
}

double calcSkewnessOnTetFaceAsAspectRatioApproachesInfinity(const std::vector<Parfait::Point<double>>& tet_in,
                                                            const std::vector<Parfait::Point<double>>& nbr,
                                                            double power) {
    Parfait::Point<double> p1, p4, p5, p6, p8;
    p1 = tet_in[0];
    p4 = tet_in[1];
    p5 = tet_in[2];
    p6 = tet_in[3];
    p8 = nbr[1];
    std::vector<Parfait::Point<double>> tet_points, neighbor_points, face;
    std::vector<int> tet_nodes{0, 1, 2, 3};
    double skewness = 1.0;
    for (int i = 0; i < 25; i++) {
        tet_points = {p1, p4, p5, p6};
        inf::Cell tet(inf::MeshInterface::TETRA_4, tet_nodes, tet_points);
        neighbor_points = {p4, p8, p5, p6};
        inf::Cell neighbor(inf::MeshInterface::TETRA_4, tet_nodes, neighbor_points);
        face = {p4, p5, p6};
        auto cj = inf::Hiro::calcCentroid(tet, power);
        auto ck = inf::Hiro::calcCentroid(neighbor, power);
        auto area_vector = calcTriangleAreaVector(face);
        area_vector.normalize();
        auto edge_vector = ck - cj;
        edge_vector.normalize();
        skewness = Parfait::Point<double>::dot(edge_vector, area_vector);
        p4[1] *= 0.5;
        p8[1] *= 0.5;
    }
    return skewness;
}

TEST_CASE("Hiro centroid for a tet") {
    Parfait::Point<double> p1, p2, p3, p4, p5, p6, p8;
    p1 = {0, 0, 0};
    p2 = {1, 0, 0};
    p3 = {1, 1, 0};
    p4 = {0, 1, 0};
    p5 = {0, 0, 1};
    p6 = {1, 0, 1};
    p8 = {0, 1, 1};

    SECTION("Recover geometric centroid when p=0") {
        std::vector<int> tet_node_ids{0, 1, 2, 3};
        std::vector<Parfait::Point<double>> tet_points{p1, p2, p3, p5};
        inf::Cell tet(inf::MeshInterface::TETRA_4, tet_node_ids, tet_points);
        double power = 0.0;
        auto centroid = inf::Hiro::calcCentroid(tet, power);
        auto geometric_centroid = calcTetGeometricCentroid(tet_points);

        tet.visualize("test-tet");

        REQUIRE(geometric_centroid[0] == Approx(centroid[0]));
        REQUIRE(geometric_centroid[1] == Approx(centroid[1]));
        REQUIRE(geometric_centroid[2] == Approx(centroid[2]));
    }

    SECTION("calculate skewness of a face in a regular tet with a given centroid for p=0,1,2") {
        std::vector<Parfait::Point<double>> tet, neighbor;
        tet = {p1, p4, p5, p6};
        neighbor = {p4, p8, p5, p6};
        double power = 0.0;
        double skewness = calcSkewnessOnTetFaceAsAspectRatioApproachesInfinity(tet, neighbor, power);
        REQUIRE(0.0 == Approx(skewness).margin(1.0e-6));
        power = 1.0;
        skewness = calcSkewnessOnTetFaceAsAspectRatioApproachesInfinity(tet, neighbor, power);
        REQUIRE(0.753563 == Approx(skewness).margin(1.0e-6));
        power = 2.0;
        skewness = calcSkewnessOnTetFaceAsAspectRatioApproachesInfinity(tet, neighbor, power);
        REQUIRE(1.0 == Approx(skewness).margin(1.0e-6));
    }
}

TEST_CASE("get edges from Cell") {
    auto mesh = inf::CartMesh::createVolume(2, 2, 2);
    inf::Cell cell(*mesh, 0);
    REQUIRE(cell.type() == inf::MeshInterface::HEXA_8);
    REQUIRE(cell.edgeCount() == 12);
    auto edges = cell.edges();
    REQUIRE(edges.size() == 12);

    SECTION("Triangle") {
        Parfait::Point<double> a = {0,0,0};
        Parfait::Point<double> b = {1,0,0};
        Parfait::Point<double> c = {1,1,0};
        inf::Cell triangle(inf::MeshInterface::TRI_3, {0,1,2}, {a, b, c});
        REQUIRE(triangle.edges().size() == 3);
    }
}

TEST_CASE("Cell should answer NODE questions") {
    auto mesh = inf::CartMesh::createWithBarsAndNodes(2, 2, 2);
    for (int c = 0; c < mesh->cellCount(); c++) {
        inf::Cell cell(mesh, c);
        REQUIRE_NOTHROW(cell.faceCount());
    }
}

TEST_CASE("Tri faces point outside the cell") {
    int dimension = 2;
    std::vector<int> cell_nodes = {0,1,2};
    std::vector<Parfait::Point<double>> cell_points = {{0,0,0}, {1,0,0}, {0,1,0}};
    inf::Cell cell(inf::MeshInterface::TRI_3, cell_nodes, cell_points, dimension);
    REQUIRE(cell.faceCount() == 3);
    auto face_0 = cell.faceAreaNormal(0);
    auto face_1 = cell.faceAreaNormal(1);
    auto face_2 = cell.faceAreaNormal(2);
    face_0.normalize();
    face_1.normalize();
    face_2.normalize();
    REQUIRE(face_0.dot({0,-1,0}) > 0.9);
    Parfait::Point<double> v = {1,1,0}; v.normalize();
    REQUIRE(face_1.dot(v) > 0.9);
    REQUIRE(face_2.dot({-1,0,0}) > 0.9);
}

TEST_CASE("Cell can have a 2D mode") {
    auto mesh = inf::CartMesh::create2DWithBarsAndNodes(2, 2);
    for (int c = 0; c < mesh->cellCount(); c++) {
        if (mesh->cellDimensionality(c) < 3) {
            inf::Cell cell(mesh, c, 2);
            if (cell.type() == inf::MeshInterface::QUAD_4) {
                REQUIRE(cell.faceCount() == 4);
                auto face_area_0 = cell.faceAreaNormal(0);
                REQUIRE(face_area_0.approxEqual(Parfait::Point<double>{0.0, -.5, 0.0}));
                auto face_area_1 = cell.faceAreaNormal(1);
                REQUIRE(face_area_1.approxEqual(Parfait::Point<double>{0.5, 0.0, 0.0}));
                auto face_area_2 = cell.faceAreaNormal(2);
                REQUIRE(face_area_2.approxEqual(Parfait::Point<double>{0.0, 0.5, 0}));
                auto face_area_3 = cell.faceAreaNormal(3);
                REQUIRE(face_area_3.approxEqual(Parfait::Point<double>{-.5, 0.0, 0}));
                REQUIRE(cell.volume() == 0.25);
            }
        }
    }
}

TEST_CASE("Cell can determine if a point is contained by the cell") {
    auto mesh = inf::CartMesh::createVolume(1, 1, 1);
    inf::Cell cell(*mesh, 0);
    REQUIRE(cell.contains(Parfait::Point<double>{0.5, 0.5, 0.5}));
    REQUIRE_FALSE(cell.contains(Parfait::Point<double>{1.5, 1.5, 1.5}));
}

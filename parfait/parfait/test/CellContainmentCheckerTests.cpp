#include <parfait/Point.h>
#include <RingAssertions.h>
#include <parfait/FileTools.h>
#include <parfait/CGNSElements.h>
#include <parfait/CellContainmentChecker.h>

TEST_CASE("check in out for tet") {
    std::array<Parfait::Point<double>, 4> tet;
    tet[0] = {0, 0, 0};
    tet[1] = {1, 0, 0};
    tet[2] = {0, 1, 0};
    tet[3] = {0, 0, 1};
    Parfait::Point<double> p = {0, 0, 0};
    REQUIRE(Parfait::CellContainmentChecker::isInCell<4>(tet, p));
    p = {0, -1, 0};
    REQUIRE_FALSE(Parfait::CellContainmentChecker::isInCell<4>(tet, p));
}

TEST_CASE("on face of anisotropic tet") {
    std::array<Parfait::Point<double>, 4> tet;

    tet[0] = {1.428809e-01, 1.000000e+00, -5.152324e-01};
    tet[1] = {1.419095e-01, 1.000000e+00, -5.136984e-01};
    tet[2] = {1.424697e-01, 1.000000e+00, -5.147030e-01};
    tet[3] = {1.424200e-01, 5.000000e-01, -5.145446e-01};

    Parfait::Point<double> point_on_face{1.421266e-01, 1.000000e+00, -5.140557e-01};
    REQUIRE(Parfait::CellContainmentChecker::isInCell<4>(tet, point_on_face));

    for (auto& vertex : tet) {
        REQUIRE(Parfait::CellContainmentChecker::isInCell<4>(tet, vertex));
    }

    auto neighbor = tet;
    neighbor[3] = {1.424200e-01, 1.500000e+00, -5.145446e-01};
    REQUIRE(Parfait::CellContainmentChecker::isInCell<4>(neighbor, point_on_face));
}

TEST_CASE("centroid of tet face") {
    std::array<Parfait::Point<double>, 4> tet;

    tet[0] = {1.54683845118294148e+01, -1.08333333333328987e+01, 2.96056856618475805e+00};
    tet[1] = {1.54740196186985095e+01, -1.08333333333328987e+01, 2.93984322097461837e+00};
    tet[2] = {1.56666659999993740e+01, -1.08333333333328987e+01, 2.94999999999988916e+00};
    tet[3] = {1.54714515127094785e+01, -1.08811909018447732e+01, 2.95289501608450999e+00};

    double third = 1.0 / 3.0;
    std::array<double, 4> weights{third, third, third, third};

    weights[3] = 0.0;
    Parfait::Point<double> p{0, 0, 0};
    for (int i = 0; i < 4; i++) p += weights[i] * tet[i];

    REQUIRE(Parfait::CellContainmentChecker::isInCell<4>(tet, p));
}

TEST_CASE("another tet face centroid") {
    std::array<Parfait::Point<double>, 4> tet;
    tet[0] = {1.10771317868462837e+01, -9.55945436079325361e+00, 2.91854614304551774e+00};
    tet[1] = {1.10355905728969041e+01, -9.52217699143721319e+00, 2.92276627791178845e+00};
    tet[2] = {1.10596818562452750e+01, -9.49764113745353278e+00, 2.89229692605977595e+00};
    tet[3] = {1.10048172240399751e+01, -9.56499113942020784e+00, 2.92500475939859594e+00};

    double third = 1.0 / 3.0;
    std::array<double, 4> weights{third, third, third, third};

    for (int k = 0; k < 4; k++) {
        auto weights_face = weights;
        weights_face[k] = 0.0;
        Parfait::Point<double> p2{0, 0, 0};
        Parfait::Point<double> p3{0, 0, 0};
        for (int i = 0; i < 4; i++) p2 += weights_face[i] * tet[i];
        REQUIRE(Parfait::CellContainmentChecker::isInCell<4>(tet, p2));
    }
}

TEST_CASE("check in out for pyramid") {
    std::array<Parfait::Point<double>, 5> pyramid;
    pyramid[0] = {0, 0, 0};
    pyramid[1] = {1, 0, 0};
    pyramid[2] = {1, 1, 0};
    pyramid[3] = {0, 1, 0};
    pyramid[4] = {0, 0, 1};
    Parfait::Point<double> p = {1, 1, 0};
    REQUIRE(Parfait::CellContainmentChecker::isInCell<5>(pyramid, p));
    p = {0, -1, 0};
    REQUIRE_FALSE(Parfait::CellContainmentChecker::isInCell<5>(pyramid, p));
}

TEST_CASE("check in out for prism") {
    std::array<Parfait::Point<double>, 6> prism;
    prism[0] = {0, 0, 0};
    prism[1] = {1, 0, 0};
    prism[2] = {0, 1, 0};
    prism[3] = {0, 0, 1};
    prism[4] = {1, 0, 1};
    prism[5] = {0, 1, 1};
    Parfait::Point<double> p = {0, 1, 1};
    REQUIRE(Parfait::CellContainmentChecker::isInCell<6>(prism, p));
    p = {0, 1.01, 1};
    REQUIRE_FALSE(Parfait::CellContainmentChecker::isInCell<6>(prism, p));
}

TEST_CASE("prism edge midpoints should always be In") {
    std::array<Parfait::Point<double>, 6> prism;
    prism[0] = {1.5195706352567007e-01, 0.0000000000000000e+00, -5.3004592372315762e-01};
    prism[1] = {1.5282115090984527e-01, 0.0000000000000000e+00, -5.3142296915015397e-01};
    prism[2] = {1.5332012062398159e-01, 0.0000000000000000e+00, -5.3232104704332261e-01};
    prism[3] = {1.5195706352567007e-01, 1.0000000000000000e+00, -5.3004592372315762e-01};
    prism[4] = {1.5282115090984527e-01, 1.0000000000000000e+00, -5.3142296915015397e-01};
    prism[5] = {1.5332012062398159e-01, 1.0000000000000000e+00, -5.3232104704332261e-01};
    Parfait::Point<double> query_point{1.5306732444344895e-01, 1.0000000000000000e+00, -5.3188819602526105e-01};
    for (auto edge_midpoint : Parfait::CGNS::Prism::computeEdgeCenters(prism)) {
        REQUIRE(Parfait::CellContainmentChecker::isInCell<6>(prism, edge_midpoint));
    }
}

TEST_CASE("check in out for hex") {
    std::array<Parfait::Point<double>, 8> hex;
    hex[0] = {0, 0, 0};
    hex[1] = {1, 0, 0};
    hex[2] = {1, 1, 0};
    hex[3] = {0, 1, 0};
    hex[4] = {0, 0, 1};
    hex[5] = {1, 0, 1};
    hex[6] = {1, 1, 1};
    hex[7] = {0, 1, 1};
    Parfait::Point<double> p = {1, 1, 1};
    REQUIRE(Parfait::CellContainmentChecker::isInCell<8>(hex, p));
    p = {0, 1.01, 1};
    REQUIRE_FALSE(Parfait::CellContainmentChecker::isInCell<8>(hex, p));
}

TEST_CASE("a special hex") {
    std::array<Parfait::Point<double>, 8> hex;
    hex[0] = {0.440000, 0.710000, 0.710000};
    hex[1] = {0.470000, 0.710000, 0.710000};
    hex[2] = {0.470000, 0.740000, 0.710000};
    hex[3] = {0.440000, 0.740000, 0.710000};
    hex[4] = {0.440000, 0.710000, 0.740000};
    hex[5] = {0.470000, 0.710000, 0.740000};
    hex[6] = {0.470000, 0.740000, 0.740000};
    hex[7] = {0.440000, 0.740000, 0.740000};

    Parfait::Point<double> p = {0.466667, 0.733333, 0.733333};
    REQUIRE(Parfait::CellContainmentChecker::isInCell<8>(hex, p));
}

// TEST_CASE("debug files"){
//    std::vector<std::string> debug_files;
//    debug_files.push_back("debug-cell.509262.3D");
//    using namespace Parfait::StringTools;
//    using namespace Parfait::FileTools;
//    int in_count = 0;
//    for(auto file:debug_files){
//        printf("File: %s\n",file.c_str());
//        auto lines = split(loadFileToString(file),"\n");
//        std::array<Parfait::Point<double>,6> prism;
//        for(int i=0;i<6;i++){
//            auto& p = prism[i];
//            auto line = split(lines[i+1]," ");
//            p[0] = toDouble(line[0]);
//            p[1] = toDouble(line[1]);
//            p[2] = toDouble(line[2]);
//        }
//        auto line = split(lines.back()," ");
//        Parfait::Point<double> query_point;
//        query_point[0] = toDouble(line[0]);
//        query_point[1] = toDouble(line[1]);
//        query_point[2] = toDouble(line[2]);
//        for(int i=0;i<6;i++) {
//            auto& vertex = prism[i];
//            printf("prism[%i] = {%.16e,%.16e,%.16e};\n", i, vertex[0], vertex[1], vertex[2]);
//        }
//        printf("query_point = {%.16e,%.16e,%.16e}\n\n",query_point[0],query_point[1],query_point[2]);
//
//        if(CellContainmentChecker::isInCell<6>(prism,query_point)) {
//            printf("____________IN___________________\n");
//            in_count++;
//        }
//        else
//            printf("____________OUT___________________\n");
//    }
//
//    REQUIRE(in_count == 1);
//}

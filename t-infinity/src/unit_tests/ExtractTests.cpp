#include <RingAssertions.h>
#include "MockMeshes.h"
#include <t-infinity/CartMesh.h>
#include <t-infinity/Extract.h>

using namespace inf;

TEST_CASE("Can extract tags") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = CartMesh::create(mp, 2, 2, 2);
    auto twod_cell_tags = extractAllTagsWithDimensionality(mp, *mesh, 2);
    REQUIRE(std::set<int>{1, 2, 3, 4, 5, 6} == twod_cell_tags);
    auto threed_cell_tags = extractAllTagsWithDimensionality(mp, *mesh, 3);
    REQUIRE(std::set<int>{0} == threed_cell_tags);
}

TEST_CASE("Can extract tag names") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = mock::getSingleHexMeshWithFaces();
    auto invertMap = [](const auto& m) {
        std::map<std::string, std::set<int>> name_to_tags;
        for (const auto& p : m) name_to_tags[p.second].insert(p.first);
        return name_to_tags;
    };
    SECTION("2D cells") {
        std::map<int, std::string> expected = {
            {1, "pikachu"}, {2, "bulbasaur"}, {3, "charmander"}, {4, "squirtle"}, {5, "pidgey"}, {6, "Ash Ketchum"}};
        for (const auto& tag_to_name : expected) mesh->setTagName(tag_to_name.first, tag_to_name.second);
        auto tag_names = extractTagsToNames(mp, *mesh, 2);
        REQUIRE(expected == tag_names);
        REQUIRE(invertMap(expected) == extractNameToTags(mp, *mesh, 2));
    }
    SECTION("3D cells") {
        std::map<int, std::string> expected = {{0, "bulbasaur"}};
        for (const auto& tag_to_name : expected) mesh->setTagName(tag_to_name.first, tag_to_name.second);
        auto tag_names = extractTagsToNames(mp, *mesh, 3);
        REQUIRE(expected == tag_names);
        REQUIRE(invertMap(expected) == extractNameToTags(mp, *mesh, 3));
    }
}

TEST_CASE("Can extract tags by name") {
   auto mp = MessagePasser(MPI_COMM_SELF);
   auto mesh = inf::CartMesh::create(2,2,2);

   auto tag_ids = inf::extractTagsByName(mp, *mesh, "Tag_1");

   REQUIRE(tag_ids.count(1)==1);
}

TEST_CASE("Can extract owned surface cell ids") {
    auto mesh = mock::getSingleHexMeshWithFaces();
    SECTION("single tag") {
        for (int tag : {1, 2, 3, 4, 5, 6}) {
            auto expected = std::vector<int>{tag - 1};
            auto actual = extractOwnedCellIdsOnTag(*mesh, tag, 2);
            REQUIRE(expected == actual);
        }
        auto hex_cell_with_zero_tag = extractOwnedCellIdsOnTag(*mesh, 0, 3);
        REQUIRE(std::vector<int>{6} == hex_cell_with_zero_tag);
    }
    SECTION("tag set") {
        SECTION("2D cells") {
            std::set<int> tags = {1, 2, 6};
            auto expected = std::vector<int>{0, 1, 5};
            auto actual = extractOwnedCellIdsInTagSet(*mesh, tags, 2);
            REQUIRE(expected == actual);
        }
        SECTION("3D cell") {
            std::set<int> tags = {0};
            auto expected = std::vector<int>{6};
            auto actual = extractOwnedCellIdsInTagSet(*mesh, tags, 3);
            REQUIRE(expected == actual);
        }
    }
}
TEST_CASE("Can extract owned surface facets") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = mock::getSingleHexMeshWithFaces();
    SECTION("All facets") {
        auto actual = extractOwned2DFacets(mp, *mesh);
        REQUIRE(12 == actual.size());

        // Verify facets are a closed, manifold surface
        Parfait::Point<double> directed_area{};
        for (const Parfait::Facet& f : actual) {
            directed_area += (f.computeNormal() * f.area());
        }
        REQUIRE(directed_area.approxEqual({0, 0, 0}));
    }
    SECTION("Facets on tag set") {
        std::set<int> tags = {1, 3};

        auto actual = extractOwned2DFacets(mp, *mesh, tags);
        REQUIRE(4 == actual.size());

        Parfait::Point<double> directed_area{};
        for (const Parfait::Facet& f : actual) {
            directed_area += (f.computeNormal() * f.area());
        }
        REQUIRE(directed_area.approxEqual({1, 0, -1}));
    }
    SECTION("Facets associated with tag name set") {
        std::set<std::string> tag_names = {"Tag_1", "Tag_3"};

        auto actual = extractOwned2DFacets(mp, *mesh, tag_names);
        REQUIRE(4 == actual.size());

        Parfait::Point<double> directed_area{};
        for (const Parfait::Facet& f : actual) {
            directed_area += (f.computeNormal() * f.area());
        }
        REQUIRE(directed_area.approxEqual({1, 0, -1}));
    }
    SECTION("Facets on tag set (with tags returned)") {
        std::set<int> tags = {1, 3};

        std::vector<Parfait::Facet> actual_facets;
        std::vector<int> actual_tags;

        std::tie(actual_facets, actual_tags) = extractOwned2DFacetsAndTheirTags(mp, *mesh, tags);
        REQUIRE(std::vector<int>{1, 1, 3, 3} == actual_tags);

        REQUIRE(4 == actual_facets.size());
        Parfait::Point<double> directed_area{};
        for (const Parfait::Facet& f : actual_facets) {
            directed_area += (f.computeNormal() * f.area());
        }
        REQUIRE(directed_area.approxEqual({1, 0, -1}));
    }
}

TEST_CASE("Can extract points") {
    auto mesh = mock::getSingleTetMesh();
    SECTION("All points") {
        auto points = extractPoints(*mesh);
        for (int node_id = 0; node_id < mesh->nodeCount(); ++node_id) {
            REQUIRE(points.at(node_id).approxEqual(mesh->node(node_id)));
        }
    }
    SECTION("Subset of points") {
        std::vector<int> node_ids = {0, 3};
        auto points = extractPoints(*mesh, node_ids);
        REQUIRE(node_ids.size() == points.size());
        REQUIRE(points.at(0).approxEqual(mesh->node(0)));
        REQUIRE(points.at(1).approxEqual(mesh->node(3)));
    }
}

TEST_CASE("Can extract owned points") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = CartMesh::create(mp, 2, 2, 2);
    auto points = extractOwnedPoints(*mesh);
    int i = 0;
    for (int node_id = 0; node_id < mesh->nodeCount(); ++node_id) {
        if (mesh->ownedNode(node_id)) {
            REQUIRE(points.at(i++).approxEqual(mesh->node(node_id)));
        }
    }
}

TEST_CASE("Can extract surface nodes") {
    auto mesh = mock::getSingleHexMeshWithFaces();
    SECTION("All surface node ids") {
        std::vector<int> all_nodes = {0, 1, 2, 3, 4, 5, 6, 7};
        for (int dimensionality = 0; dimensionality < 3; ++dimensionality) {
            auto expected = dimensionality > 1 ? all_nodes : std::vector<int>{};
            auto actual = extractAllNodeIdsInCellsWithDimensionality(*mesh, dimensionality);
            REQUIRE(expected == actual);
        }
    }
    SECTION("Node ids in tag") {
        int tag = 2;
        std::vector<int> expected = {0, 1, 4, 5};
        auto actual = extractNodeIdsViaDimensionalityAndTag(*mesh, tag, 2);
        REQUIRE(expected == actual);
        REQUIRE(expected == extractOwnedNodeIdsViaDimensionalityAndTag(*mesh, tag, 2));
        REQUIRE(extractNodeIdsViaDimensionalityAndTag(*mesh, tag, 3).empty());
        REQUIRE(extractOwnedNodeIdsViaDimensionalityAndTag(*mesh, tag, 3).empty());
    }
}

TEST_CASE("Can extract do-own cell") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = CartMesh::create(mp, 2, 2, 2);
    auto do_own = extractDoOwnCells(*mesh);
    REQUIRE(mesh->cellCount() == (int)do_own.size());
    for (int cell_id = 0; cell_id < mesh->cellCount(); ++cell_id) {
        REQUIRE(mesh->ownedCell(cell_id) == do_own[cell_id]);
    }
}

TEST_CASE("Can extract cell centroids") {
    auto mesh = mock::getSingleTetMesh();
    auto centroids = extractCellCentroids(*mesh);
    REQUIRE(mesh->cellCount() == (int)centroids.size());
    for (int cell_id = 0; cell_id < mesh->cellCount(); ++cell_id) {
        std::vector<int> cell_nodes;
        mesh->cell(cell_id, cell_nodes);

        Parfait::Point<double> average_center{};
        for (int node_id : cell_nodes) average_center += mesh->node(node_id);
        average_center /= double(cell_nodes.size());

        REQUIRE(average_center.approxEqual(centroids.at(cell_id)));
    }
}

TEST_CASE("Can get global ids") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = CartMesh::create(mp, 2, 2, 2);
    SECTION("Nodes") {
        auto actual = extractGlobalNodeIds(*mesh);
        auto actual_owned = extractOwnedGlobalNodeIds(*mesh);
        int i = 0;
        for (int node_id = 0; node_id < mesh->nodeCount(); ++node_id) {
            REQUIRE(mesh->globalNodeId(node_id) == actual.at(node_id));
            if (mesh->ownedNode(node_id)) {
                REQUIRE(mesh->globalNodeId(node_id) == actual_owned.at(i++));
            }
        }
    }
    SECTION("Cells") {
        auto actual = extractGlobalCellIds(*mesh);
        auto actual_owned = extractOwnedGlobalCellIds(*mesh);
        int i = 0;
        for (int cell_id = 0; cell_id < mesh->cellCount(); ++cell_id) {
            REQUIRE(mesh->globalCellId(cell_id) == actual.at(cell_id));
            if (mesh->ownedCell(cell_id)) {
                REQUIRE(mesh->globalCellId(cell_id) == actual_owned.at(i++));
            }
        }
    }
}

TEST_CASE("Can extract owning ranks") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = CartMesh::create(mp, 2, 2, 2);
    SECTION("Nodes") {
        auto owners = extractNodeOwners(*mesh);
        for (int node_id = 0; node_id < mesh->nodeCount(); ++node_id) {
            REQUIRE(mesh->nodeOwner(node_id) == owners.at(node_id));
        }
    }
    SECTION("Cells") {
        auto owners = extractCellOwners(*mesh);
        for (int cell_id = 0; cell_id < mesh->cellCount(); ++cell_id) {
            REQUIRE(mesh->cellOwner(cell_id) == owners.at(cell_id));
        }
    }
}

TEST_CASE("Can extract surface area") {
    auto mesh = mock::getSingleTetMesh();
    auto areas = extractSurfaceArea(*mesh);
    REQUIRE(5 == areas.size());
    REQUIRE(areas.at(0).approxEqual({0, 0, -0.5}));
    REQUIRE(areas.at(1).approxEqual({0, -0.5, 0}));
    REQUIRE(areas.at(2).approxEqual({0.5, 0.5, 0.5}));
    REQUIRE(areas.at(3).approxEqual({-0.5, 0, 0}));
    REQUIRE(areas.at(4).approxEqual({0, 0, 0}));
}

TEST_CASE("Can get the average normal of surface elements by tag") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    std::set<int> tags = {1};
    SECTION("2D") {
        auto mesh = TinfMesh(mock::triangleWithBar(), 0);
        Parfait::Point<double> expected_normal = {0, -1, 0};
        auto average_normal = extractAverageNormal(mp, mesh, tags);
        REQUIRE(expected_normal.approxEqual(average_normal));
    }
    SECTION("3D") {
        auto mesh = mock::getSingleHexMeshWithFaces();
        Parfait::Point<double> expected_normal = {0, 0, -1};
        auto average_normal = extractAverageNormal(mp, *mesh, tags);
        REQUIRE(expected_normal.approxEqual(average_normal));
    }
}
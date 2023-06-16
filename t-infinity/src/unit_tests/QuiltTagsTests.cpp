#include <RingAssertions.h>
#include <set>
#include <vector>
#include <map>
#include <t-infinity/QuiltTags.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Cell.h>

TEST_CASE("quilt tags, but don't flatten") {
    std::set<int> current_tags = {0, 1, 2, 3, 4, 5, 6};
    std::vector<std::vector<int>> tags_to_merge = {{0, 3}};
    auto old_to_new = inf::quiltTagsDontCompact(tags_to_merge, current_tags);
    REQUIRE(old_to_new.size() == current_tags.size());

    REQUIRE(old_to_new.at(0) == 0);
    REQUIRE(old_to_new.at(1) == 1);
    REQUIRE(old_to_new.at(2) == 2);
    REQUIRE(old_to_new.at(3) == 0);
    REQUIRE(old_to_new.at(4) == 4);
    REQUIRE(old_to_new.at(5) == 5);
    REQUIRE(old_to_new.at(6) == 6);
}

TEST_CASE("quilt tags and flatten to ordinals") {
    std::set<int> current_tags = {0, 1, 2, 3, 4, 5, 6};
    std::vector<std::vector<int>> tags_to_merge = {{0, 3}};
    auto old_to_new = inf::quiltTagsAndCompact(tags_to_merge, current_tags);
    REQUIRE(old_to_new.size() == current_tags.size());

    REQUIRE(old_to_new.at(0) == 1);
    REQUIRE(old_to_new.at(1) == 2);
    REQUIRE(old_to_new.at(2) == 3);
    REQUIRE(old_to_new.at(3) == 1);
    REQUIRE(old_to_new.at(4) == 4);
    REQUIRE(old_to_new.at(5) == 5);
    REQUIRE(old_to_new.at(6) == 6);
}

TEST_CASE("quilt tags and flatten to ordinals from sparse") {
    std::set<int> current_tags = {0, 1, 2, 3, 5, 6};
    std::vector<std::vector<int>> tags_to_merge = {{0, 3}, {}};
    auto old_to_new = inf::quiltTagsAndCompact(tags_to_merge, current_tags);
    REQUIRE(old_to_new.size() == current_tags.size());

    REQUIRE(old_to_new.at(0) == 1);
    REQUIRE(old_to_new.at(1) == 2);
    REQUIRE(old_to_new.at(2) == 3);
    REQUIRE(old_to_new.at(3) == 1);
    REQUIRE(old_to_new.at(5) == 4);
    REQUIRE(old_to_new.at(6) == 5);
}

TEST_CASE("Relabel tags by nearest normal") {
    auto cart_mesh = inf::CartMesh::create(1, 1, 1);
    auto rewrite_all_tags = inf::extract2DTags(*cart_mesh);
    std::map<int, Parfait::Point<double>> tags_to_normals;
    tags_to_normals[100] = {0, 0, -1};
    tags_to_normals[101] = {0, -1, 0};
    tags_to_normals[102] = {1, 0, 0};
    tags_to_normals[103] = {0, 1, 0};
    tags_to_normals[104] = {-1, 0, 0};
    tags_to_normals[105] = {0, 0, 1};

    inf::relabelSurfaceTagsByNearestNormal(*cart_mesh, tags_to_normals, rewrite_all_tags);

    auto sameDirection = [](Parfait::Point<double> n1, Parfait::Point<double> n2) {
        n1.normalize();
        n2.normalize();
        auto dot = Parfait::Point<double>::dot(n1, n2);
        if (dot >= 0.95) return true;
        return false;
    };

    int num_surfaces_checked = 0;
    for (int c = 0; c < cart_mesh->cellCount(); c++) {
        if (cart_mesh->is2DCell(c)) {
            inf::Cell cell(*cart_mesh, c);
            if (sameDirection(cell.faceAreaNormal(0), {0, 0, -1})) {
                REQUIRE(cart_mesh->cellTag(c) == 100);
            } else if (sameDirection(cell.faceAreaNormal(0), {0, -1, 0})) {
                REQUIRE(cart_mesh->cellTag(c) == 101);
            } else if (sameDirection(cell.faceAreaNormal(0), {1, 0, 0})) {
                REQUIRE(cart_mesh->cellTag(c) == 102);
            } else if (sameDirection(cell.faceAreaNormal(0), {0, 1, 0})) {
                REQUIRE(cart_mesh->cellTag(c) == 103);
            } else if (sameDirection(cell.faceAreaNormal(0), {-1, 0, 0})) {
                REQUIRE(cart_mesh->cellTag(c) == 104);
            } else if (sameDirection(cell.faceAreaNormal(0), {0, 0, 1})) {
                REQUIRE(cart_mesh->cellTag(c) == 105);
            }
            num_surfaces_checked++;
        }
    }

    REQUIRE(num_surfaces_checked == 6);
}

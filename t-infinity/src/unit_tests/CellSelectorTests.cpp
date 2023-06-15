#include <RingAssertions.h>
#include <t-infinity/CellSelector.h>
#include "MockMeshes.h"

using namespace inf;

void requireKeptCellCount(const MeshInterface& mesh, const CellSelector& selector, int expected) {
    int n_kept = 0;
    for (int i = 0; i < mesh.cellCount(); i++)
        if (selector.keep(mesh, i)) n_kept++;
    REQUIRE(expected == n_kept);
}

TEST_CASE("create a cell selector") {
    auto mesh_ptr = inf::mock::getSingleTetMesh();
    auto& mesh = *mesh_ptr;

    int n_volume_cells = mesh.cellCount(MeshInterface::TETRA_4);
    int n_surface_cells = mesh.cellCount(MeshInterface::TRI_3);
    int n_total_cells = n_volume_cells + n_surface_cells;

    SECTION("select by element type") {
        CellTypeSelector selector_1(MeshInterface::TRI_3);
        requireKeptCellCount(mesh, selector_1, n_surface_cells);

        CellTypeSelector selector_2(MeshInterface::TETRA_4);
        requireKeptCellCount(mesh, selector_2, n_volume_cells);

        CellTypeSelector selector_3({MeshInterface::TRI_3, MeshInterface::TETRA_4});
        requireKeptCellCount(mesh, selector_3, n_surface_cells + n_volume_cells);
    }

    SECTION("select surface elements") {
        DimensionalitySelector selector(2);
        requireKeptCellCount(mesh, selector, n_surface_cells);
    }

    SECTION("select volume elements") {
        DimensionalitySelector selector(3);
        requireKeptCellCount(mesh, selector, n_volume_cells);
    }

    SECTION("select based on extent") {
        SECTION("non overlapping") {
            Parfait::Extent<double> non_overlapping_extent({10, 10, 10}, {11, 11, 11});
            ExtentSelector selector(non_overlapping_extent);
            requireKeptCellCount(mesh, selector, 0);
        }
        SECTION("overlapping") {
            Parfait::Extent<double> overlapping_extent({0, 0, 0}, {1, 1, 1});
            ExtentSelector selector(overlapping_extent);
            requireKeptCellCount(mesh, selector, n_total_cells);
        }
    }

    SECTION("select based on sphere") {
        SECTION("non overlapping") {
            SphereSelector selector(1, {10, 10, 10});
            requireKeptCellCount(mesh, selector, 0);
        }
        SECTION("overlapping") {
            SphereSelector selector(1, {0, 0, 0});
            requireKeptCellCount(mesh, selector, n_total_cells);
        }
    }

    SECTION("select cell based on values at nodes") {
        std::vector<double> values{2, 3, 3, 4};
        SECTION("all cells have at least one node in the range") {
            NodeValueSelector selector(0, 3, values);
            requireKeptCellCount(mesh, selector, 5);
        }
        SECTION("only one node is in the range") {
            NodeValueSelector selector(2, 2, values);
            requireKeptCellCount(mesh, selector, 4);
        }
        SECTION("no nodes are in the range") {
            NodeValueSelector selector(5, 10, values);
            requireKeptCellCount(mesh, selector, 0);
        }
    }

    SECTION("select cell based on values in cells") {
        std::vector<double> values{0, 1, 2, 3, 4};
        SECTION("all cells are in the range") {
            CellValueSelector selector(0, 4, values);
            requireKeptCellCount(mesh, selector, n_total_cells);
        }
        SECTION("some of the cells are in the range") {
            CellValueSelector selector(0, 3, values);
            requireKeptCellCount(mesh, selector, 4);
        }
        SECTION("none of the cells are in the range") {
            CellValueSelector selector(10, 10, values);
            requireKeptCellCount(mesh, selector, 0);
        }
    }

    SECTION("composite selector") {
        CompositeSelector selector;
        requireKeptCellCount(mesh, selector, n_total_cells);

        selector.add(std::make_shared<DimensionalitySelector>(2));
        requireKeptCellCount(mesh, selector, n_surface_cells);

        selector.add(std::make_shared<DimensionalitySelector>(3));
        requireKeptCellCount(mesh, selector, 0);
    }
}

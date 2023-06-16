#pragma once
#include <parfait/CellContainmentChecker.h>
#include <MessagePasser/MessagePasser.h>
#include <Tracer.h>
#include <WeightBasedInterpolator.h>
#include <parfait/Adt3dExtent.h>
#include <parfait/ExtentBuilder.h>
#include <parfait/Point.h>
#include <parfait/Throw.h>
#include <stdio.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Cell.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/TinfMeshAppender.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/MeshLoader.h>
#include <t-infinity/RepartitionerInterface.h>
#include <iostream>
#include <t-infinity/Communicator.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/FieldTools.h>
#include <t-infinity/CellIdFilter.h>
#include <t-infinity/FieldExtractor.h>
#include <t-infinity/Snap.h>
#include <t-infinity/NullCommand.h>
#include <parfait/PointWriter.h>
#include <t-infinity/MeshShuffle.h>

std::tuple<std::vector<Parfait::Extent<double>>, std::vector<int>> cellExtentsInDomain(
    const inf::MeshInterface& mesh, const Parfait::Extent<double>& domain) {
    std::vector<Parfait::Extent<double>> extents;
    std::vector<int> cell_ids;
    std::vector<int> cell;
    for (int i = 0; i < mesh.cellCount(); i++) {
        if (mesh.is3DCellType(mesh.cellType(i))) {
            cell.resize(mesh.cellLength(i));
            mesh.cell(i, cell.data());
            auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
            for (int id : cell) {
                Parfait::Point<double> p = mesh.node(id);
                Parfait::ExtentBuilder::add(e, p);
            }
            if (domain.intersects(e)) {
                extents.push_back(e);
                cell_ids.push_back(i);
            }
        }
    }
    return {extents, cell_ids};
}

std::shared_ptr<Parfait::Adt3DExtent> buildAdtForOverlappingCells(const inf::MeshInterface& mesh,
                                                                  const std::vector<Parfait::Point<double>>& points) {
    auto domain = Parfait::ExtentBuilder::build(points);
    std::vector<Parfait::Extent<double>> extents;
    std::vector<int> cell_ids;
    std::tie(extents, cell_ids) = cellExtentsInDomain(mesh, domain);
    auto adt_domain = Parfait::ExtentBuilder::build(extents);
    auto adt = std::make_shared<Parfait::Adt3DExtent>(adt_domain);
    for (size_t i = 0; i < cell_ids.size(); i++) {
        auto e = extents[i];
        auto id = cell_ids[i];
        adt->store(id, e);
    }
    return adt;
}

bool isInCellAndSetWeights(std::vector<Parfait::Point<double>> cell_points,
                           const std::vector<int>& cell_nodes,
                           Parfait::Point<double> p,
                           YOGA::DonorCloud& stencil) {
    int n = int(cell_points.size());
    stencil.node_ids.resize(n);
    stencil.weights.resize(n);
    bool found = false;
    switch (n) {
        case 4:
            if (Parfait::CellContainmentChecker::isInCell_c(cell_points.front().data(), n, p.data())) {
                YOGA::FiniteElementInterpolation<double>::calcWeightsTet(
                    cell_points.front().data(), p.data(), stencil.weights.data());
                stencil.node_ids = cell_nodes;
                found = true;
            }
            break;
        case 5:
            if (Parfait::CellContainmentChecker::isInCell_c(cell_points.front().data(), n, p.data())) {
                YOGA::FiniteElementInterpolation<double>::calcWeightsPyramid(
                    cell_points.front().data(), p.data(), stencil.weights.data());
                stencil.node_ids = cell_nodes;
                found = true;
            }
            break;
        case 6:
            if (Parfait::CellContainmentChecker::isInCell_c(cell_points.front().data(), n, p.data())) {
                YOGA::FiniteElementInterpolation<double>::calcWeightsPrism(
                    cell_points.front().data(), p.data(), stencil.weights.data());
                stencil.node_ids = cell_nodes;
                found = true;
            }
            break;
        case 8:
            if (Parfait::CellContainmentChecker::isInCell_c(cell_points.front().data(), n, p.data())) {
                YOGA::FiniteElementInterpolation<double>::calcWeightsHex(
                    cell_points.front().data(), p.data(), stencil.weights.data());
                stencil.node_ids = cell_nodes;
                found = true;
            }
            break;
    }
    return found;
}

double calculateAmountOutsideBounds(Parfait::Point<double> p, double min = 0, double max = 1.0) {
    double amount = 0;
    for (int i = 0; i < 3; i++) {
        if (p[i] < min) {
            double delta = p[i] - min;
            amount += std::fabs(delta);
        }
        if (p[i] > max) {
            amount += std::fabs(p[i] - max);
        }
    }
    return amount;
}

void make3DFilesofCells(const inf::MeshInterface& mesh,
                        const std::vector<int>& cells,
                        const Parfait::Point<double>& p) {
    for (auto cell_id : cells) {
        inf::Cell cell(mesh, cell_id);
        auto points = cell.points();
        std::vector<double> field(points.size(), 0.0);
        points.push_back(p);
        field.push_back(1.0);
        Parfait::PointWriter::write("debug-cell." + std::to_string(cell_id), points, field);
    }
}

YOGA::DonorCloud findBestDonorCell(const inf::MeshInterface& mesh,
                                   const std::vector<int>& donor_candidates,
                                   Parfait::Point<double> p) {
    bool found = false;
    YOGA::DonorCloud stencil;
    int best_donor = donor_candidates.front();
    for (int cell_id : donor_candidates) {
        inf::Cell cell(mesh, cell_id);
        found = isInCellAndSetWeights(cell.points(), cell.nodes(), p, stencil);
        if (found) {
            best_donor = cell_id;
            break;
        }
    }

    if (not found) {
        // make3DFilesofCells(mesh, donor_candidates, p);
        // PARFAIT_THROW("Unable to find `is in` donor.\n");
        double min_amount_wrong = 1e200;
        for (int cell_id : donor_candidates) {
            inf::Cell cell(mesh, cell_id);
            auto rst = YOGA::FiniteElementInterpolation<double>::calcComputationalCoordinates(
                cell.points().size(), cell.points().front().data(), p.data());
            double amount_wrong = calculateAmountOutsideBounds(rst);
            if (amount_wrong < min_amount_wrong) {
                min_amount_wrong = amount_wrong;
                best_donor = cell_id;
            }
        }
    }

    inf::Cell cell(mesh, best_donor);
    YOGA::FiniteElementInterpolation<double>::calcWeights(
        cell.points().size(), cell.points().front().data(), p.data(), stencil.weights.data());
    stencil.node_ids = cell.nodes();

    return stencil;
}

std::vector<YOGA::DonorCloud> generateDonorStencilsForPoints(const inf::MeshInterface& mesh,
                                                             const std::vector<Parfait::Point<double>>& points) {
    std::vector<YOGA::DonorCloud> donor_stencils(points.size());
    Tracer::begin("building adt");
    auto adt = buildAdtForOverlappingCells(mesh, points);
    Tracer::end("building adt");

    Tracer::begin("searching adt");
    for (size_t i = 0; i < points.size(); i++) {
        auto p = points[i];
        auto donor_candidates = adt->retrieve({p, p});
        if (donor_candidates.size() == 0) PARFAIT_THROW("couldn't find donor.");
        donor_stencils[i] = findBestDonorCell(mesh, donor_candidates, p);
    }
    Tracer::end("searching adt");
    return donor_stencils;
}

std::string getVisualizationPluginName(const std::vector<std::string>& arguments) {
    std::string name = "ParfaitViz";
    for (size_t i = 0; i < arguments.size(); i++) {
        auto arg = arguments[i];
        if (arg == "--output-plugin" or arg == "--viz-plugin") {
            name = arguments[i + 1];
        }
    }
    return name;
}

std::vector<double> convertFieldToVector(inf::FieldInterface& f) {
    std::vector<double> out(f.size());
    for (size_t n = 0; n < out.size(); n++) {
        f.value(n, &out[n]);
    }
    return out;
}

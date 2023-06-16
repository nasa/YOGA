#include "FieldTools.h"
#include "GhostSyncer.h"
#include "parfait/Checkpoint.h"
#include <parfait/Throw.h>
#include <parfait/TaubinSmoothing.h>
#include <parfait/LaplacianSmoothing.h>
#include <parfait/Flatten.h>
#include "MeshConnectivity.h"
#include "VectorFieldAdapter.h"
#include "MDVector.h"

namespace inf {
namespace FieldTools {
    std::vector<std::vector<int>> buildStencils(const MeshInterface& mesh,
                                                const std::string& association) {
        std::vector<std::vector<int>> stencil;
        if (association == inf::FieldAttributes::Node()) {
            stencil = inf::NodeToNode::buildForAnyNodeInSharedCell(mesh);
        } else if (association == inf::FieldAttributes::Cell()) {
            stencil = inf::CellToCell::build(mesh);
        } else {
            PARFAIT_THROW("Can only build stencils for smoothing on Node or Cell fields.");
        }
        return stencil;
    }
    std::vector<double> findMin(MessagePasser mp,
                                const MeshInterface& mesh,
                                const FieldInterface& f) {
        auto type = f.association();

        std::vector<double> min(f.blockSize(), std::numeric_limits<double>::max());
        if (type == FieldAttributes::Node()) {
            for (int n = 0; n < mesh.nodeCount(); n++) {
                if (mesh.ownedNode(n)) {
                    auto values = f.at(n);
                    for (int i = 0; i < f.blockSize(); ++i) min[i] = std::min(values[i], min[i]);
                }
            }
        } else if (type == FieldAttributes::Cell()) {
            for (int c = 0; c < mesh.cellCount(); c++) {
                if (mesh.ownedCell(c)) {
                    auto values = f.at(c);
                    for (int i = 0; i < f.blockSize(); ++i) min[i] = std::min(values[i], min[i]);
                }
            }
        } else {
            PARFAIT_THROW("Not implemented for field association: " + type);
        }

        for (auto& v : min) v = mp.ParallelMin(v);
        return min;
    }
    std::vector<double> findMax(MessagePasser mp,
                                const MeshInterface& mesh,
                                const FieldInterface& f) {
        auto type = f.association();

        std::vector<double> max(f.blockSize(), std::numeric_limits<double>::min());
        if (type == FieldAttributes::Node()) {
            for (int n = 0; n < mesh.nodeCount(); n++) {
                if (mesh.ownedNode(n)) {
                    auto values = f.at(n);
                    for (int i = 0; i < f.blockSize(); ++i) max[i] = std::max(values[i], max[i]);
                }
            }
        } else if (type == FieldAttributes::Cell()) {
            for (int c = 0; c < mesh.cellCount(); c++) {
                if (mesh.ownedCell(c)) {
                    auto values = f.at(c);
                    for (int i = 0; i < f.blockSize(); ++i) max[i] = std::max(values[i], max[i]);
                }
            }
        } else {
            PARFAIT_THROW("Not implemented for field association: " + type);
        }

        for (auto& v : max) v = mp.ParallelMax(v);
        return max;
    }
    double findNodalMin(MessagePasser mp, const MeshInterface& mesh, const std::vector<double>& f) {
        double min = std::numeric_limits<double>::max();
        for (int n = 0; n < mesh.nodeCount(); n++) {
            if (mesh.nodeOwner(n) == mp.Rank()) {
                min = std::min(f[n], min);
            }
        }

        return mp.ParallelMin(min);
    }
    std::vector<double> getScalarFieldAsVector(const FieldInterface& f) {
        PARFAIT_ASSERT(f.blockSize() == 1, "Expected scalar field, but blockSize is > 1");
        std::vector<double> out(f.size());

        for (int i = 0; i < f.size(); i++) {
            double d;
            f.value(i, &d);
            out[i] = d;
        }

        return out;
    }
    std::vector<double> getFieldAsVector(const FieldInterface& f, int entry_index) {
        std::vector<double> out(f.size());

        std::vector<double> values(f.blockSize());
        for (int i = 0; i < f.size(); i++) {
            values = f.at(i);
            out[i] = values[entry_index];
        }

        return out;
    }
    std::shared_ptr<inf::FieldInterface> cellToNode_volume(
        const MeshInterface& mesh,
        const FieldInterface& f,
        const std::vector<std::vector<int>>& n2c) {
        PARFAIT_ASSERT(f.size() == mesh.cellCount(),
                       "Field and Mesh cell counts don't match.\nfield:" +
                           std::to_string(f.size()) + " mesh: " + std::to_string(mesh.cellCount()));
        PARFAIT_ASSERT(f.blockSize() == 1, "Expected scalar field, but blockSize is > 1");
        auto getCellField = [&](int c) {
            double d;
            f.value(c, &d);
            return d;
        };
        auto node_field = calcNodeFromVolumeCellAverage(mesh, getCellField, n2c);
        return std::make_shared<inf::VectorFieldAdapter>(
            f.name(), inf::FieldAttributes::Node(), 1, node_field);
    }
    std::shared_ptr<inf::FieldInterface> cellToNode_volume(const MeshInterface& mesh,
                                                           const FieldInterface& f) {
        if (f.size() != mesh.cellCount()) {
            PARFAIT_THROW("Field and Mesh cell counts don't match.\nfield:" +
                          std::to_string(f.size()) + " mesh: " + std::to_string(mesh.cellCount()));
        }
        auto n2c = inf::NodeToCell::build(mesh);
        return cellToNode_volume(mesh, f, n2c);
    }
    std::shared_ptr<inf::VectorFieldAdapter> cellToNode_simpleAverage(
        const MeshInterface& mesh,
        const FieldInterface& f,
        const std::vector<std::vector<int>>& n2c) {
        PARFAIT_ASSERT(f.size() == mesh.cellCount(),
                       "Field and Mesh cell counts don't match.\nfield:" +
                           std::to_string(f.size()) + " mesh: " + std::to_string(mesh.cellCount()));
        int bs = f.blockSize();
        std::vector<double> node_field(mesh.nodeCount() * f.blockSize());
        std::vector<double> temp(bs);
        for (int node = 0; node < mesh.nodeCount(); node++) {
            auto& neighbors = n2c.at(node);
            for (auto neighbor : neighbors) {
                f.value(neighbor, temp.data());
                for (int e = 0; e < f.blockSize(); e++) {
                    node_field[bs * node + e] += temp[e];
                }
            }
            for (int e = 0; e < f.blockSize(); e++) {
                node_field[bs * node + e] /= double(neighbors.size());
            }
        }
        return std::make_shared<inf::VectorFieldAdapter>(
            f.name(), inf::FieldAttributes::Node(), f.blockSize(), node_field);
    }
    std::shared_ptr<inf::VectorFieldAdapter> cellToNode_simpleAverage(const MeshInterface& mesh,
                                                                      const FieldInterface& f) {
        PARFAIT_ASSERT(f.size() == mesh.cellCount(),
                       "Field and Mesh cell counts don't match.\nfield:" +
                           std::to_string(f.size()) + " mesh: " + std::to_string(mesh.cellCount()));
        auto n2c = inf::NodeToCell::build(mesh);
        return cellToNode_simpleAverage(mesh, f, n2c);
    }
    std::shared_ptr<inf::FieldInterface> cellToNode_surface(
        const MeshInterface& mesh,
        const FieldInterface& f,
        const std::vector<std::vector<int>>& n2c) {
        if (f.size() != mesh.cellCount()) {
            PARFAIT_THROW("Field and Mesh cell counts don't match.\nfield:" +
                          std::to_string(f.size()) + " mesh: " + std::to_string(mesh.cellCount()));
        }
        PARFAIT_ASSERT(f.blockSize() == 1, "vector field not implemented");
        auto getCellField = [&](int c) {
            double d;
            f.value(c, &d);
            return d;
        };
        auto node_field = calcNodeFromSurfaceCellAverage(mesh, getCellField, n2c);
        return std::make_shared<inf::VectorFieldAdapter>(
            f.name(), inf::FieldAttributes::Node(), 1, node_field);
    }
    std::shared_ptr<inf::FieldInterface> cellToNode_surface(const MeshInterface& mesh,
                                                            const FieldInterface& f) {
        if (f.size() != mesh.cellCount()) {
            PARFAIT_THROW("Field and Mesh cell counts don't match.\nfield:" +
                          std::to_string(f.size()) + " mesh: " + std::to_string(mesh.cellCount()));
        }
        auto n2c = inf::NodeToCell::build(mesh);
        return cellToNode_surface(mesh, f, n2c);
    }
    std::shared_ptr<inf::FieldInterface> nodeToCell(const MeshInterface& mesh,
                                                    const FieldInterface& f) {
        PARFAIT_ASSERT(f.association() == FieldAttributes::Node(),
                       "Cannot transform field at " + f.association() + " using nodeToCell");
        PARFAIT_ASSERT(f.size() == mesh.nodeCount(),
                       "Field and Mesh node counts don't match.\nfield:" +
                           std::to_string(f.size()) + " mesh: " + std::to_string(mesh.nodeCount()));
        std::vector<double> cell_field(mesh.cellCount() * f.blockSize(), 0.0);
        std::vector<int> nodes;
        std::vector<double> temp(f.blockSize());
        nodes.reserve(8);
        for (int cell = 0; cell < mesh.cellCount(); cell++) {
            mesh.cell(cell, nodes);
            for (auto node : nodes) {
                f.value(node, temp.data());
                for (int e = 0; e < f.blockSize(); e++) {
                    cell_field[f.blockSize() * cell + e] += temp[e];
                }
            }
            for (int e = 0; e < f.blockSize(); e++) {
                cell_field[f.blockSize() * cell + e] /= double(nodes.size());
            }
        }
        return std::make_shared<inf::VectorFieldAdapter>(
            f.name(), inf::FieldAttributes::Cell(), f.blockSize(), cell_field);
    }
    std::shared_ptr<inf::FieldInterface> taubinSmoothing(
        const MessagePasser& mp,
        std::shared_ptr<MeshInterface> mesh,
        std::shared_ptr<FieldInterface> f,
        const std::vector<std::vector<int>>& stencil,
        Parfait::Dictionary options) {
        Parfait::Dictionary defaults;
        defaults["passes"] = 20;
        defaults["damping weight"] = 1.0;

        options = defaults.overrideEntries(options);

        GhostSyncer syncer(mp, mesh);
        auto sync = [&](auto& v) {
            if (f->association() == FieldAttributes::Node()) {
                syncer.syncNodes(v);
            } else {
                syncer.syncCells(v);
            }
        };
        std::vector<std::vector<double>> values;
        for (int entry = 0; entry < f->blockSize(); ++entry) {
            values.push_back(getFieldAsVector(*f, entry));
            sync(values.back());
        }
        std::vector<double> damping_weights(f->size(), options.at("damping weight").asDouble());
        for (int pass = 0; pass < options.at("passes").asInt(); ++pass) {
            for (auto& entry : values) {
                Parfait::TaubinSmoothing::taubinSmoothingPass(
                    double(0.0), stencil, entry, damping_weights, 0.33);
                sync(entry);
                Parfait::TaubinSmoothing::taubinSmoothingPass(
                    double(0.0), stencil, entry, damping_weights, -0.34);
                sync(entry);
            }
        }
        return std::make_shared<inf::VectorFieldAdapter>(
            f->name(), f->association(), f->blockSize(), Parfait::flatten(values));
    }

    std::vector<double> extractColumn(FieldInterface& input, int column) {
        std::vector<double> temp(input.blockSize());
        std::vector<double> output(input.size());
        for (int item = 0; item < input.size(); item++) {
            input.value(item, temp.data());
            output[item] = temp[column];
        }
        return output;
    }
    std::vector<std::shared_ptr<inf::FieldInterface>> split(
        std::shared_ptr<inf::FieldInterface> input) {
        std::vector<std::string> names(input->blockSize());
        for (int i = 0; i < input->blockSize(); i++) {
            names[i] = input->name() + "_" + std::to_string(i);
        }
        return split(input, names);
    }
    std::vector<std::shared_ptr<inf::FieldInterface>> split(
        std::shared_ptr<inf::FieldInterface> input, std::vector<std::string> names) {
        if (int(names.size()) != input->blockSize()) {
            PARFAIT_WARNING("Cannot assign names of split field of length " +
                            std::to_string(names.size()) + " when field has " +
                            std::to_string(input->blockSize()) + " columns");
            return split(input);
        }
        std::vector<std::shared_ptr<inf::FieldInterface>> output;
        for (int i = 0; i < input->blockSize(); i++) {
            auto column_vector = extractColumn(*input, i);
            auto ptr = std::make_shared<VectorFieldAdapter>(
                names[i], input->association(), 1, column_vector);
            output.push_back(ptr);
        }
        return output;
    }
    std::shared_ptr<inf::FieldInterface> merge(
        std::vector<std::shared_ptr<inf::FieldInterface>> fields, std::string name) {
        PARFAIT_ASSERT(fields.size() > 0, "Cannot merge zero fields");

        int size = fields.front()->size();
        auto at = fields.front()->association();

        int num_columns = fields.size();
        for (auto& f : fields) {
            PARFAIT_ASSERT(f->association() == at, "Cannot merge fields of different associations");
            PARFAIT_ASSERT(f->blockSize() == 1, "Cannot merge non scalar fields");
            PARFAIT_ASSERT(f->size() == size, "Cannot merge fields of different lengths");
        }

        std::vector<double> output_data(size * num_columns);

        for (int c = 0; c < int(fields.size()); c++) {
            auto& f = fields[c];
            for (int n = 0; n < size; n++) {
                output_data[num_columns * n + c] = f->getDouble(n, 0);
            }
        }

        return std::make_shared<inf::VectorFieldAdapter>(name, at, num_columns, output_data);
    }

    std::vector<double> gatherNodalScalarField(MessagePasser mp,
                                               const MeshInterface& mesh,
                                               const FieldInterface& field,
                                               int root) {
        PARFAIT_ASSERT(field.blockSize() == 1, "vector field not implemented");
        std::map<long, double> my_field;
        for (int n = 0; n < mesh.nodeCount(); n++) {
            if (mesh.nodeOwner(n) == mp.Rank()) {
                auto global = mesh.globalNodeId(n);
                double d;
                field.value(n, &d);
                my_field[global] = d;
            }
        }
        return mp.GatherByIds(my_field, root);
    }
    std::vector<double> gatherCellScalarField(MessagePasser mp,
                                              const MeshInterface& mesh,
                                              const FieldInterface& field,
                                              int root) {
        PARFAIT_ASSERT(field.blockSize() == 1, "vector field not implemented");
        std::map<long, double> my_field;
        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.cellOwner(c) == mp.Rank()) {
                auto global = mesh.globalCellId(c);
                double d;
                field.value(c, &d);
                my_field[global] = d;
            }
        }
        return mp.GatherByIds(my_field, root);
    }
    std::vector<double> gatherScalarField(MessagePasser mp,
                                          const MeshInterface& mesh,
                                          const FieldInterface& field,
                                          int root) {
        auto association = field.association();
        if (association == inf::FieldAttributes::Node()) {
            return gatherNodalScalarField(mp, mesh, field, root);
        } else if (association == inf::FieldAttributes::Cell()) {
            return gatherCellScalarField(mp, mesh, field, root);
        }
        PARFAIT_THROW("Cannot gather field " + field.name() +
                      "with unknown association: " + field.association());
    }
    std::vector<double> gatherField(MessagePasser mp,
                                    const inf::MeshInterface& mesh,
                                    const inf::FieldInterface& field,
                                    int root) {
        PARFAIT_ASSERT(field.blockSize() == 1, "Can only gather scalar field");
        return gatherScalarField(mp, mesh, field, root);
    }

    std::vector<double> gatherNodeField(MessagePasser mp,
                                        const inf::MeshInterface& mesh,
                                        const std::vector<double>& node_field,
                                        int root) {
        std::map<long, double> my_field;
        for (int n = 0; n < mesh.nodeCount(); n++) {
            if (mesh.nodeOwner(n) == mp.Rank()) {
                auto global = mesh.globalNodeId(n);
                my_field[global] = node_field[n];
            }
        }
        return mp.GatherByIds(my_field, root);
    }

    std::vector<double> gatherCellField(MessagePasser mp,
                                        const inf::MeshInterface& mesh,
                                        const std::vector<double>& cell_field,
                                        int root) {
        std::map<long, double> my_field;
        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.cellOwner(c) == mp.Rank()) {
                auto global = mesh.globalCellId(c);
                my_field[global] = cell_field[c];
            }
        }
        return mp.GatherByIds(my_field, root);
    }
    std::vector<double> fillAtNodes(const MeshInterface& mesh,
                                    std::function<double(double, double, double)> function_of_xyz) {
        std::vector<double> field(mesh.nodeCount());
        for (int n = 0; n < mesh.nodeCount(); n++) {
            auto p = mesh.node(n);
            field[n] = function_of_xyz(p[0], p[1], p[2]);
        }
        return field;
    }
    std::shared_ptr<inf::FieldInterface> fillFieldAtNodes(
        const MeshInterface& mesh,
        std::function<double(double, double, double)> function_of_xyz,
        std::string name) {
        auto f = fillAtNodes(mesh, function_of_xyz);
        return std::make_shared<inf::VectorFieldAdapter>(name, inf::FieldAttributes::Node(), 1, f);
    }
    std::shared_ptr<inf::FieldInterface> laplacianSmoothing(const MeshInterface& mesh,
                                                            const FieldInterface& f,
                                                            int passes) {
        return laplacianSmoothing(mesh, f, buildStencils(mesh, f.association()), passes);
    }
    std::shared_ptr<inf::FieldInterface> laplacianSmoothing(
        const MeshInterface& mesh,
        const FieldInterface& f,
        const std::vector<std::vector<int>>& stencil,
        int passes) {
        MDVector<double, 2> values({f.size(), f.blockSize()});
        for (int entry = 0; entry < f.blockSize(); ++entry) {
            auto field = getFieldAsVector(f, entry);
            for (int pass = 0; pass < passes; ++pass)
                Parfait::laplacianSmoothing(0.0, stencil, field);
            for (int i = 0; i < f.size(); ++i) values(i, entry) = field[i];
        }
        return std::make_shared<inf::VectorFieldAdapter>(
            f.name(), f.association(), f.blockSize(), values.vec);
    }
    std::shared_ptr<FieldInterface> op(const std::string& name,
                                       const FieldInterface& f1,
                                       const FieldInterface& f2,
                                       FieldOperation operation) {
        PARFAIT_ASSERT_EQUAL(f1.size(), f2.size());
        PARFAIT_ASSERT_EQUAL(f1.blockSize(), f2.blockSize());
        PARFAIT_ASSERT(f1.association() == f2.association(),
                       "Cannot values fields with different associations");
        PARFAIT_ASSERT_EQUAL(f1.size(), f2.size());
        int bs = f1.blockSize();

        std::vector<double> scratch_1(f1.blockSize());
        std::vector<double> scratch_2(f1.blockSize());
        std::vector<double> values(f1.size() * f1.blockSize());
        for (int i = 0; i < f1.size(); i++) {
            f1.value(i, scratch_1.data());
            f2.value(i, scratch_2.data());
            for (long j = 0; j < f1.blockSize(); j++) {
                values[i * bs + j] = operation(scratch_1[j], scratch_2[j]);
            }
        }

        return std::make_shared<inf::VectorFieldAdapter>(name, f1.association(), bs, values);
    }
    std::shared_ptr<FieldInterface> op(const std::string& name,
                                       const FieldInterface& f,
                                       ScalarFieldOperation operation) {
        int bs = f.blockSize();

        std::vector<double> scratch(f.blockSize());
        std::vector<double> values(f.size() * f.blockSize());
        for (int i = 0; i < f.size(); i++) {
            f.value(i, scratch.data());
            for (long j = 0; j < f.blockSize(); j++) {
                values[i * bs + j] = operation(scratch[j]);
            }
        }

        return std::make_shared<inf::VectorFieldAdapter>(name, f.association(), bs, values);
    }

    std::shared_ptr<FieldInterface> getOpWithName(const FieldInterface& f1,
                                                  const FieldInterface& f2,
                                                  const std::string& op_name,
                                                  FieldOperation field_operation) {
        auto name = f1.name() + op_name + f2.name();
        name = Parfait::StringTools::findAndReplace(name, " ", ".");
        return op(name, f1, f2, field_operation);
    }

    std::shared_ptr<FieldInterface> diff(const FieldInterface& f1, const FieldInterface& f2) {
        return getOpWithName(f1, f2, "-", [](const auto& a, const auto& b) { return a - b; });
    }
    std::shared_ptr<FieldInterface> sum(const FieldInterface& f1, const FieldInterface& f2) {
        return getOpWithName(f1, f2, "+", [](const auto& a, const auto& b) { return a + b; });
    }
    std::shared_ptr<FieldInterface> multiply(const FieldInterface& f1, const FieldInterface& f2) {
        return getOpWithName(f1, f2, "*", [](const auto& a, const auto& b) { return a * b; });
    }
    std::shared_ptr<FieldInterface> divide(const FieldInterface& f1, const FieldInterface& f2) {
        return getOpWithName(f1, f2, "/", [](const auto& a, const auto& b) { return a / b; });
    }
    std::shared_ptr<FieldInterface> diff(const FieldInterface& f1, double scalar) {
        return op(f1.name(), f1, [=](const auto& a) { return a - scalar; });
    }
    std::shared_ptr<FieldInterface> sum(const FieldInterface& f1, double scalar) {
        return op(f1.name(), f1, [=](const auto& a) { return a + scalar; });
    }
    std::shared_ptr<FieldInterface> multiply(const FieldInterface& f1, double scalar) {
        return op(f1.name(), f1, [=](const auto& a) { return a * scalar; });
    }
    std::shared_ptr<FieldInterface> divide(const FieldInterface& f1, double scalar) {
        return op(f1.name(), f1, [=](const auto& a) { return a / scalar; });
    }
    std::shared_ptr<FieldInterface> abs(const FieldInterface& f1) {
        return op(f1.name(), f1, [=](const auto& a) { return std::fabs(a); });
    }

    double norm(MessagePasser mp,
                const std::vector<bool>& do_own,
                const inf::FieldInterface& f,
                double p) {
        double norm = 0.0;
        int n = f.blockSize();
        std::vector<double> entry(n);
        for (int i = 0; i < f.size(); i++) {
            if (do_own[i]) {
                f.value(i, entry.data());
                for (double v : entry) {
                    norm += std::pow(v, p);
                }
            }
        }

        norm += mp.ParallelSum(norm);
        return std::pow(norm, 1. / p);
    }
    double spaldingYPlus(double uplus) {
        return uplus + 0.1108 * (std::exp(0.4 * uplus) - 1.0 - 0.4 * uplus);
    }
    double spaldingYPlusDerivative(double uplus) {
        return 1.0 + 0.1108 * (exp(0.4 * uplus) * 0.4 - 0.4);
    }
    double calcSpaldingLawOfTheWallUPlus(double yplus) {
        double uplus = yplus > 12.0 ? std::log(yplus / 0.1108) / 0.4 : yplus;
        for (int iteration = 0; iteration < 100; ++iteration) {
            auto y = spaldingYPlus(uplus);
            auto error = y - yplus;
            auto dyplus_duplus = spaldingYPlusDerivative(uplus);
            uplus -= error / dyplus_duplus;
            if (std::abs(y) > 1e-3) {
                if (std::abs(error / y) < 1e-12) return uplus;
            } else {
                if (std::abs(error) < 1e-15) return uplus;
            }
        }
        PARFAIT_THROW("Spalding Newton solve failed");
    }
    std::vector<double> calcSpaldingLawOfTheWallUPlus(double yplus,
                                                      const std::vector<double>& wall_distance) {
        std::vector<double> uplus;
        uplus.reserve(wall_distance.size());
        for (double distance : wall_distance) {
            uplus.push_back(calcSpaldingLawOfTheWallUPlus(distance / yplus));
        }
        return uplus;
    }

    std::shared_ptr<inf::FieldInterface> smoothField(const MessagePasser& mp,
                                                     const std::string& type,
                                                     std::shared_ptr<FieldInterface> field,
                                                     std::shared_ptr<MeshInterface> mesh,
                                                     const Parfait::Dictionary& options) {
        GhostSyncer syncer(mp, mesh);
        std::shared_ptr<FieldInterface> smooth_field;
        auto stencils = buildStencils(*mesh, field->association());
        if (type == "taubin")
            return FieldTools::taubinSmoothing(mp, mesh, field, stencils, options);

        int passes = options.has("passes") ? options.at("passes").asInt() : 20;

        for (int pass = 0; pass < passes; ++pass) {
            if (type == "laplacian") {
                smooth_field = FieldTools::laplacianSmoothing(*mesh, *field, stencils, 1);
            } else {
                PARFAIT_THROW("Unknown smoothing type requested: " + type);
            }
            auto f = dynamic_cast<VectorFieldAdapter&>(*smooth_field);
            if (field->association() == FieldAttributes::Node()) {
                syncer.syncNodes(f.getVector());
            } else {
                syncer.syncCells(f.getVector());
            }
        }
        return smooth_field;
    }
    std::shared_ptr<VectorFieldAdapter> createNodeFieldFromCallback(
        std::string name,
        const MeshInterface& mesh,
        std::function<double(double, double, double)> callback) {
        std::vector<double> field(mesh.nodeCount());
        for (int n = 0; n < mesh.nodeCount(); n++) {
            auto p = mesh.node(n);
            field[n] = callback(p[0], p[1], p[2]);
        }
        return std::make_shared<VectorFieldAdapter>(name, FieldAttributes::Node(), field);
    }
    std::shared_ptr<FieldInterface> rename(const std::string& name, const FieldInterface& f) {
        return op(name, f, [](double v) { return v; });
    }
}
}
#pragma once
#include <t-infinity/MeshBuilder.h>
#include <t-infinity/Geometry.h>
#include <t-infinity/CartMesh.h>
#include <parfait/MetricTensor.h>
#include <memory>

struct MeshBuilderAndGeometry {
    inf::experimental::MeshBuilder builder;
    std::shared_ptr<inf::geometry::DatabaseInterface> db;
};

inline MeshBuilderAndGeometry getInitialUnitQuad(MessagePasser mp) {
    auto cart_mesh = inf::CartMesh::create2DWithBarsAndNodes(1, 1);
    auto builder = inf::experimental::MeshBuilder(mp, std::move(cart_mesh));

    std::vector<std::shared_ptr<inf::geom_impl::PiecewiseP1Edge>> edges;
    auto bar_type = inf::MeshInterface::BAR_2;
    auto& bars = builder.mesh->mesh.cells.at(bar_type);
    int num_bars = bars.size() / 2;
    int next_cad_edge_id = 0;
    int next_cad_node = 0;
    for (int bar_index = 0; bar_index < num_bars; bar_index++) {
        int node_0 = bars[2 * bar_index + 0];
        int node_1 = bars[2 * bar_index + 1];
        Parfait::Point<double> a = builder.mesh->node(node_0);
        Parfait::Point<double> b = builder.mesh->node(node_1);
        edges.push_back(
            std::make_shared<inf::geom_impl::PiecewiseP1Edge>(std::vector<Parfait::Point<double>>{a, b}));
        inf::geometry::GeomAssociation g;
        g.type = inf::geometry::GeomAssociation::EDGE;
        g.index = next_cad_edge_id++;
        builder.node_geom_association[node_0].push_back(g);
        builder.node_geom_association[node_1].push_back(g);

        g.type = inf::geometry::GeomAssociation::NODE;
        g.index = next_cad_node++;
        builder.node_geom_association[node_0].push_back(g);
    }
    builder.initializeConnectivity();
    auto curve_database = std::make_shared<inf::geom_impl::PiecewiseP1DataBase>();
    curve_database->curves = edges;
    return {builder, curve_database};
}

inline std::function<double(Parfait::Point<double>, Parfait::Point<double>)> buildRadialCallback(double u) {
    auto metric_at_point = [=](Parfait::Point<double> p_orig) {
        auto p = p_orig;
        double radius = p.magnitude();
        p.normalize();
        double h = (radius - 0.08);
        h = h*h;
        h *= 5*u;

        Parfait::Point<double> z = {0,0,1};
        auto t = p.cross(z);
        p *= h;
        t.normalize();
        t*= 5*h;
        auto m = Parfait::MetricTensor::metricFromEllipse({p}, {t}, {0, 0, h});
        if(not m.isFinite()){
            PARFAIT_THROW("Invalid metric tensor at point " + p_orig.to_string());
        }
        return m;
    };

    auto calc_edge_length = [=](Parfait::Point<double> a, Parfait::Point<double> b) {
        std::vector<Parfait::Tensor> metrics;
        std::vector<double> weights;
        int num_steps = 10;
        double dx = 1.0 / num_steps;
        for (int s = 0; s < num_steps; s++) {
            double alpha = s * dx;
            double beta = 1.0 - alpha;
            auto p = alpha * a + beta * b;
            metrics.push_back(metric_at_point(p));
            weights.push_back(1.0);
        }
        double norm = 0.0;
        for (auto w : weights) {
            norm += abs(w);
        }
        for (auto& w : weights) {
            w /= norm;
        }
        auto m = Parfait::MetricTensor::logEuclideanAverage(metrics, weights);
        return Parfait::MetricTensor::edgeLength(m, (a - b));
    };

    return calc_edge_length;
}

inline std::function<double(Parfait::Point<double>, Parfait::Point<double>)> buildAnisotropicCallback(double u) {
    auto metric_at_point = [=](Parfait::Point<double> p) {
        double h = u;
        return Parfait::MetricTensor::metricFromEllipse({h, 0, 0}, {0, 0.2 * h, 0}, {0, 0, h});
    };

    auto calc_edge_length = [=](Parfait::Point<double> a, Parfait::Point<double> b) {
        std::vector<Parfait::Tensor> metrics;
        std::vector<double> weights;
        int num_steps = 10;
        double dx = 1.0 / num_steps;
        for (int s = 0; s < num_steps; s++) {
            double alpha = s * dx;
            double beta = 1.0 - alpha;
            auto p = alpha * a + beta * b;
            metrics.push_back(metric_at_point(p));
            weights.push_back(1.0);
        }
        double norm = 0.0;
        for (auto w : weights) {
            norm += abs(w);
        }
        for (auto& w : weights) {
            w /= norm;
        }
        auto m = Parfait::MetricTensor::logEuclideanAverage(metrics, weights);
        return Parfait::MetricTensor::edgeLength(m, (a - b));
    };

    return calc_edge_length;
}

inline std::function<double(Parfait::Point<double>, Parfait::Point<double>)> buildIsotropicCallback(double h) {
    auto metric_at_point = [=](Parfait::Point<double> p) {
        return Parfait::MetricTensor::metricFromEllipse({h, 0, 0}, {0, h, 0}, {0, 0, h});
    };

    auto calc_edge_length = [=](Parfait::Point<double> a, Parfait::Point<double> b) {
        std::vector<Parfait::Tensor> metrics;
        std::vector<double> weights;
        int num_steps = 10;
        double dx = 1.0 / num_steps;
        for (int s = 0; s < num_steps; s++) {
            double alpha = s * dx;
            double beta = 1.0 - alpha;
            auto p = alpha * a + beta * b;
            metrics.push_back(metric_at_point(p));
            weights.push_back(1.0);
        }
        double norm = 0.0;
        for (auto w : weights) {
            norm += abs(w);
        }
        for (auto& w : weights) {
            w /= norm;
        }
        auto m = Parfait::MetricTensor::logEuclideanAverage(metrics, weights);
        return Parfait::MetricTensor::edgeLength(m, (a - b));
    };

    return calc_edge_length;
}

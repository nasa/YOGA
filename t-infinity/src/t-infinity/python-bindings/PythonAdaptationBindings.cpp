#include "PybindIncludes.h"

#include <MessagePasser/MessagePasser.h>
#include <t-infinity/MetricCalculatorInterface.h>
#include <t-infinity/MeshAdaptionInterface.h>
#include <t-infinity/InterpolationInterface.h>
#include <t-infinity/MetricManipulator.h>
#include <t-infinity/MetricAugmentationFactory.h>
#include <t-infinity/GeometryAssociationLoaderInterface.h>
#include <t-infinity/ByteStreamLoaderInterface.h>
#include <t-infinity/StructuredMeshAdaptationInterface.h>
#include <t-infinity/StructuredTinfMesh.h>
#include <t-infinity/Extract.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/VectorFieldAdapter.h>

using namespace inf;
namespace py = pybind11;

void PythonAdaptationBindings(py::module& m) {
    typedef std::shared_ptr<MeshInterface> Mesh;
    typedef std::shared_ptr<FieldInterface> Field;
    typedef std::shared_ptr<MetricCalculatorInterface> MetricCalculator;

    py::class_<MetricManipulator>(m, "MetricManipulator")
        .def("calcImpliedMetricAtNodes",
             [](MessagePasser mp, Mesh mesh) {
                 auto dimensionality = maxCellDimensionality(mp, *mesh);
                 return MetricManipulator::calcImpliedMetricAtNodes(*mesh, dimensionality);
             })
        .def("calcImpliedMetricAtCells",
             [](MessagePasser mp, Mesh mesh) {
                 auto dimensionality = maxCellDimensionality(mp, *mesh);
                 return MetricManipulator::calcImpliedMetricAtCells(*mesh, dimensionality);
             })
        .def("calcUniformMetricAtNodes",
             [](Mesh mesh) { return MetricManipulator::calcUniformMetricAtNodes(*mesh); })
        .def("calcComplexity",
             [](MessagePasser mp, Mesh mesh, Field metric) {
                 return MetricManipulator::calcComplexity(mp, *mesh, *metric);
             })
        .def("scaleToTargetComplexity",
             [](MessagePasser mp, Mesh mesh, Field metric, double target_complexity) {
                 return MetricManipulator::scaleToTargetComplexity(
                     mp, *mesh, *metric, target_complexity);
             })
        .def("intersect",
             [](Field metric_a, Field metric_b) {
                 return MetricManipulator::intersect(*metric_a, *metric_b);
             })
        .def("setPrescribedSpacingOnMetric",
             [](MessagePasser mp,
                Mesh mesh,
                Field metric,
                std::set<int> tags,
                double target_complexity,
                double target_spacing) {
                 return MetricManipulator::setPrescribedSpacingOnMetric(
                     mp, *mesh, *metric, tags, target_complexity, target_spacing);
             })
        .def("limitEdgeLengthInRegion",
             [](Mesh mesh, Field metric, std::vector<int> node_ids, double min_edge_length) {
                 return MetricManipulator::limitMinimumEdgeLength(
                     *mesh, *metric, node_ids, min_edge_length);
             })
        .def("blendMetricsViaWallDistance",
             [](double wall_distance_threshold,
                double blending_power,
                std::vector<double> node_wall_distances,
                Mesh mesh,
                Field metric_near_wall,
                Field metric_away_from_wall) {
                 auto blending = std::make_shared<DistanceBlendedMetric>(
                     wall_distance_threshold,
                     node_wall_distances,
                     MetricManipulator::fromFieldToTensors(*metric_near_wall),
                     blending_power);
                 return MetricManipulator::overrideMetric(
                     extractPoints(*mesh), *metric_away_from_wall, {blending});
             })
        .def("clipMetric",
             [](Field target_metric, Field source_metric) {
                 PARFAIT_ASSERT(target_metric->association() == FieldAttributes::Node() and
                                    source_metric->association() == FieldAttributes::Node(),
                                "Metric must be at nodes for clipping");
                 ClipEigenvalues clipping(MetricManipulator::fromFieldToTensors(*target_metric));
                 auto out = MetricManipulator::fromFieldToTensors(*source_metric);
                 for (int i = 0; i < (int)out.size(); ++i)
                     out[i] = clipping.targetMetricAt({}, i, out[i]);
                 return MetricManipulator::toNodeField(out);
             })
        .def("applyWallSpacingMetric",
             [](double target_wall_spacing,
                double growth_rate,
                double wall_distance_threshold,
                double tangent_spacing,
                std::vector<double> node_wall_distances,
                MessagePasser mp,
                Mesh mesh,
                Field metric) -> Field {
                 auto explicit_wall_spacing =
                     std::make_shared<ExplicitWallSpacingMetricBlending>(target_wall_spacing,
                                                                         growth_rate,
                                                                         wall_distance_threshold,
                                                                         tangent_spacing,
                                                                         node_wall_distances,
                                                                         mp,
                                                                         mesh);
                 return MetricManipulator::overrideMetric(
                     extractPoints(*mesh), *metric, {explicit_wall_spacing});
             })
        .def("shrinkWallSpacingMetric",
             [](double target_wall_spacing,
                double growth_rate,
                double wall_distance_threshold,
                double tangent_spacing,
                std::vector<double> node_wall_distances,
                MessagePasser mp,
                Mesh mesh,
                Field metric) -> Field {
                 auto explicit_wall_spacing =
                     std::make_shared<ShrinkMultiscale>(target_wall_spacing,
                                                        growth_rate,
                                                        wall_distance_threshold,
                                                        tangent_spacing,
                                                        node_wall_distances,
                                                        mp,
                                                        mesh);
                 return MetricManipulator::intersect(
                     extractPoints(*mesh), *metric, {explicit_wall_spacing});
             });

    py::class_<MetricCalculatorInterface, MetricCalculator>(m, "MetricCalculator")
        .def(py::init([](std::string directory, std::string name) {
            return getMetricCalculator(directory, name);
        }))
        .def("calculate",
             [](MetricCalculator self,
                Mesh mesh,
                MessagePasser mp,
                Field field,
                std::string parameters) {
                 return self->calculate(mesh, mp.getCommunicator(), field, parameters);
             })
        .def("calculateFromScalarAverage",
             [](MetricCalculator self,
                Mesh mesh,
                MessagePasser mp,
                std::vector<Field> fields,
                std::string parameters) {
                 return self->calculateFromMultiple(mesh, mp.getCommunicator(), fields, parameters);
             })
        .def("unloadPlugin", [](MetricCalculator self) { self.reset(); });

    typedef std::shared_ptr<GeometryAssociationLoaderInterface> GeometryAssociationLoader;
    py::class_<GeometryAssociationLoaderInterface, GeometryAssociationLoader>(
        m, "GeometryAssociationLoader")
        .def(py::init([](std::string directory, std::string name) {
            return getGeometryAssociationLoader(directory, name);
        }))
        .def("load",
             [](GeometryAssociationLoader self, MessagePasser mp, std::string filename, Mesh mesh) {
                 return self->load(mp.getCommunicator(), filename, mesh);
             });

    typedef std::shared_ptr<GeometryAssociationInterface> GeometryAssociation;
    py::class_<GeometryAssociationInterface, GeometryAssociation>(m, "GeometryAssociation")
        .def("associationCount", &GeometryAssociationInterface::associationCount);

    typedef std::shared_ptr<ByteStreamLoaderInterface> ByteStreamLoader;
    py::class_<ByteStreamLoaderInterface, ByteStreamLoader>(m, "ByteStreamLoader")
        .def(py::init([](std::string directory, std::string name) {
            return getByteStreamLoader(directory, name);
        }))
        .def("load", [](ByteStreamLoader self, std::string filename, MessagePasser mp) {
            return self->load(filename, mp.getCommunicator());
        });

    typedef std::shared_ptr<ByteStreamInterface> ByteStream;
    py::class_<ByteStreamInterface, ByteStream>(m, "ByteStream")
        .def("size", &ByteStreamInterface::size);

    typedef MeshAdaptationInterface::MeshAssociatedWithGeometry MeshAssociatedWithGeometry;
    py::class_<MeshAssociatedWithGeometry>(m, "MeshAssociatedWithGeometry")
        .def(py::init())
        .def_readwrite("mesh", &MeshAssociatedWithGeometry::mesh)
        .def_readwrite("mesh_geometry_association",
                       &MeshAssociatedWithGeometry::mesh_geometry_association);

    typedef std::shared_ptr<MeshAdaptationInterface> MeshAdapter;
    py::class_<MeshAdaptationInterface, MeshAdapter>(m, "MeshAdaptationPlugin")
        .def(py::init([](std::string directory, std::string name) {
            return getMeshAdaptationPlugin(directory, name);
        }))
        .def("adaptWithGeometry",
             [](MeshAdapter self,
                MeshAssociatedWithGeometry mesh_associated_with_geometry,
                ByteStream geometry_model,
                MessagePasser mp,
                Field metric,
                std::string json_options) {
                 return self->adapt(mesh_associated_with_geometry,
                                    geometry_model,
                                    mp.getCommunicator(),
                                    metric,
                                    json_options);
             })
        .def("adapt",
             [](MeshAdapter self,
                Mesh mesh,
                MessagePasser mp,
                Field metric,
                std::string json_options) {
                 return self->adapt(mesh, mp.getCommunicator(), metric, json_options);
             })

        .def("unloadPlugin", [](MeshAdapter self) { self.reset(); });

    typedef std::shared_ptr<StructuredMeshAdaptationInterface> StructuredMeshAdapter;
    py::class_<StructuredMeshAdaptationInterface, StructuredMeshAdapter>(
        m, "StructuredMeshAdaptationPlugin")
        .def(py::init([](std::string directory, std::string name) {
            return getStructuredMeshAdaptationPlugin(directory, name);
        }))
        .def("adapt",
             [](StructuredMeshAdapter self,
                int block_id,
                std::shared_ptr<StructuredTinfMesh> mesh,
                Field field,
                std::string options) {
                 self->adapt(mesh->getBlock(block_id), *mesh->getField(block_id, *field), options);
             });

    typedef std::shared_ptr<InterpolationInterface> Interpolator;
    py::class_<InterpolationInterface, Interpolator>(m, "Interpolation")
        .def(py::init([](std::string directory,
                         std::string name,
                         Mesh donor,
                         Mesh receptor,
                         MessagePasser mp) {
            return getInterpolator(directory, name, mp.getCommunicator(), donor, receptor);
        }))
        .def("interpolate", &InterpolationInterface::interpolate);
}

from . import _infinity_plugins as infinity
from . import plugin_helpers as helpers
import json


class RefineMeshAdapter(object):
    def __init__(self, comm):
        self.__plugin_directory = helpers.get_plugin_library_path()
        self.__mesh_adapter = infinity.MeshAdaptationPlugin(self.__plugin_directory, "RefinePlugins")
        self.__metric_calculator = infinity.MetricCalculator(self.__plugin_directory, "RefinePlugins")
        self.__geometry_association_loader = infinity.GeometryAssociationLoader(self.__plugin_directory,
                                                                                "RefinePlugins")
        self.__geometry_model_loader = infinity.ByteStreamLoader(self.__plugin_directory, "RefinePlugins")
        self.__comm = comm

    def importMeshWithGeometryAssociation(self, meshb_filename):
        m = infinity.MeshAssociatedWithGeometry()
        m.mesh = helpers.import_mesh("RefinePlugins", meshb_filename, self.__comm)
        m.mesh_geometry_association = self.__geometry_association_loader.load(self.__comm, meshb_filename, m.mesh)
        return m

    def importGeometryModel(self, meshb_filename):
        return self.__geometry_model_loader.load(meshb_filename, self.__comm)

    def adaptToComplexity(self, mesh, field, complexity, adaptation_options):
        metric_options = json.loads(adaptation_options)
        metric_options["complexity"] = complexity
        metric = self.__metric_calculator.calculate(mesh, self.__comm, field, json.dumps(metric_options))
        return self.__mesh_adapter.adapt(mesh, self.__comm, metric, adaptation_options)

    def adapt(self, mesh_associated_with_geometry, geometry_model, metric, adaptation_options):
        return self.__mesh_adapter.adaptWithGeometry(mesh_associated_with_geometry, geometry_model, self.__comm, metric,
                                                     adaptation_options)

    def calculateMetric(self, mesh, field, options):
        return self.__metric_calculator.calculate(mesh, self.__comm, field, options)

    def adaptWithGeometry(self, mesh_associated_with_geometry, geometry_model, field, adaptation_options):
        metric = self.__metric_calculator.calculate(mesh_associated_with_geometry.mesh, self.__comm, field,
                                                    adaptation_options)
        return self.adapt(mesh_associated_with_geometry, geometry_model, metric, adaptation_options)

    def interpolateField(self, donor, receptor, field):
        interpolator = infinity.Interpolation(helpers.get_plugin_library_path(), "RefinePlugins", donor, receptor,
                                              self.__comm)
        return interpolator.interpolate(field)

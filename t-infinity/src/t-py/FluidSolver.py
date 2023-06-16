from . import _infinity_plugins as infinity
from . import plugin_helpers as helpers
import json


class PyFluidSolver(infinity.FluidSolver):
    def __init__(self, name, config, mesh, comm=infinity.getCommunicator()):
        infinity.FluidSolver.__init__(
            self, mesh, comm, config, helpers.get_plugin_library_path(), name)
        self.mesh = mesh
        self.comm = comm

    def field(self, scalars_in_field, field_name=None):
        if type(scalars_in_field) == str:
            scalars_in_field = [scalars_in_field]
        if not field_name:
            field_name = scalars_in_field[0]
        scalars_in_field = list(scalars_in_field)
        if self.isNodeField(scalars_in_field):
            return infinity.extractScalarsAsNodeField(self, self.mesh, field_name, scalars_in_field)
        elif self.isCellField(scalars_in_field):
            return infinity.extractScalarsAsCellField(self, self.mesh, field_name, scalars_in_field)
        else:
            raise RuntimeError("Unable to create field %s containing: " % field_name + str(scalars_in_field))

    def visualizeVolume(self, output_name="domain", fieldnames=None, viz_plugin_name="ParfaitViz"):
        fields = [self.field(f) for f in self.__getFieldNames(fieldnames)]

        viz = infinity.PyViz(output_name, self.mesh, self.comm, helpers.get_plugin_library_path(), viz_plugin_name)
        for f in fields:
            viz.add(f)
        viz.visualize()

    def visualizeSampling(self, config: dict, fieldnames=None):
        # Currently on supported fully for node fields
        fieldnames = self.__getFieldNames(fieldnames)
        fields = [self.field(name) for name in fieldnames]
        infinity.visualizeFromDictionary(self.comm, self.mesh, fields, json.dumps(config))

    def visualizeSurface(self, surface_tags, output_name="domain-surface", fieldnames=None,
                         viz_plugin_name="ParfaitViz"):
        surface_filter = infinity.TagFilter(self.comm, self.mesh, surface_tags)
        fields = [surface_filter.apply(self.field(f)) for f in self.__getFieldNames(fieldnames)]

        allowable_cell_types = [self.mesh.BAR_2] if self.mesh.is2D(self.comm) else [self.mesh.TRI_3, self.mesh.QUAD_4]
        cell_type_filter = infinity.CellTypeFilter(self.comm, surface_filter.getMesh(), allowable_cell_types)

        viz = infinity.PyViz(output_name, cell_type_filter.getMesh(), self.comm, helpers.get_plugin_library_path(),
                             viz_plugin_name)
        for f in [cell_type_filter.apply(f) for f in fields]:
            viz.add(f)
        viz.visualize()

    def getSolution(self):
        return self.field(self.expectsSolutionAs(), field_name="solution")

    def setSolution(self, solution_field):
        if self.isNodeField(self.expectsSolutionAs()):
            for j in range(self.mesh.nodeCount()):
                self.setSolutionAtNode(j, solution_field.at(j))
        elif self.isCellField(self.expectsSolutionAs()):
            for j in range(self.mesh.cellCount()):
                self.setSolutionAtCell(j, solution_field.at(j))

    def readRestart(self, filename="restart.snap"):
        infinity.setSolutionFromSnap(filename, self.mesh, self, self.comm)

    def writeRestart(self, filename="restart.snap"):
        solution_fields = self.expectsSolutionAs()
        self.__writeSnapFile(filename, solution_fields)

    def writeSnapFromFieldNames(self, field_names, filename="restart.snap"):
        self.__writeSnapFile(filename, field_names)

    def isNodeField(self, scalars):
        return all(s in self.listScalarsAtNodes() for s in scalars)

    def isCellField(self, scalars):
        return all(s in self.listScalarsAtCells() for s in scalars)

    def __writeSnapFile(self, filename, fields):
        infinity.writeSolverFieldsToSnap(
            filename, self.mesh, self, fields, self.comm)

    def __getFieldNames(self, names):
        return list(names) if names else self.listFields()

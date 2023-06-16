from unittest import TestCase
from os import path
import infinity_plugins as infinity
import json

one_ring_root = path.realpath(path.join(__file__, "../../../.."))
plugin_path = infinity.get_plugin_library_path()

hs_config = {
    "states": {
        "farfield": {
            "densities": [1.225],
            "speed": 285.43,
            "angle of attack": 1.0,
            "temperature": 285.15
        }
    },
    "print configuration": False,
    "boundary conditions": [
        {"mesh boundary tags": [1, 2, 3, 4, 5, 6], "type": "tangency"}
    ]
}


def loadGrid(grid_filename):
    grid_path = path.join(one_ring_root, "grids/lb8.ugrid")
    return infinity.import_mesh("RefinePlugins", path.join(grid_path, grid_filename))


class TestInfinityPlugins(TestCase):
    @classmethod
    def setUpClass(cls):
        infinity.initializeMPI()

    @classmethod
    def tearDownClass(cls):
        infinity.finalizeMPI()

    def test_Field(self):
        field = infinity.Field("charmander", [1, 2, 3])
        self.assertEqual(field.name(), "charmander")
        self.assertEqual(field.at(0)[0], 1.0)
        self.assertEqual(field.at(1)[0], 2.0)
        self.assertEqual(field.at(2)[0], 3.0)
        self.assertEqual(field.size(), 3)

    def test_Field_diff(self):
        field = infinity.Field("f", [1, 2, 3])
        result = field - field
        self.assertEqual(result.name(), "f-f")
        self.assertEqual(result.at(0)[0], 0.0)
        self.assertEqual(result.at(1)[0], 0.0)
        self.assertEqual(result.at(2)[0], 0.0)
        self.assertEqual(result.size(), 3)

        result = field - 1.0
        self.assertEqual(result.name(), "f")
        self.assertEqual(result.at(0)[0], 0.0)
        self.assertEqual(result.at(1)[0], 1.0)
        self.assertEqual(result.at(2)[0], 2.0)
        self.assertEqual(result.size(), 3)

    def test_Field_sum(self):
        field = infinity.Field("f", [1, 2, 3])
        result = field + field
        self.assertEqual(result.name(), "f+f")
        self.assertEqual(result.at(0)[0], 2.0)
        self.assertEqual(result.at(1)[0], 4.0)
        self.assertEqual(result.at(2)[0], 6.0)
        self.assertEqual(result.size(), 3)

        result = field + 1.0
        self.assertEqual(result.name(), "f")
        self.assertEqual(result.at(0)[0], 2.0)
        self.assertEqual(result.at(1)[0], 3.0)
        self.assertEqual(result.at(2)[0], 4.0)
        self.assertEqual(result.size(), 3)

    def test_Field_multiply(self):
        field = infinity.Field("f", [1, 2, 3])
        result = field * field
        self.assertEqual(result.name(), "f*f")
        self.assertEqual(result.at(0)[0], 1.0)
        self.assertEqual(result.at(1)[0], 4.0)
        self.assertEqual(result.at(2)[0], 9.0)
        self.assertEqual(result.size(), 3)

        result = field * 2.0
        self.assertEqual(result.name(), "f")
        self.assertEqual(result.at(0)[0], 2.0)
        self.assertEqual(result.at(1)[0], 4.0)
        self.assertEqual(result.at(2)[0], 6.0)
        self.assertEqual(result.size(), 3)

    def test_Field_div(self):
        field = infinity.Field("f", [1, 2, 3])
        result = field / field
        self.assertEqual(result.name(), "f/f")
        self.assertEqual(result.at(0)[0], 1.0)
        self.assertEqual(result.at(1)[0], 1.0)
        self.assertEqual(result.at(2)[0], 1.0)
        self.assertEqual(result.size(), 3)

        result = field / 2.0
        self.assertEqual(result.name(), "f")
        self.assertEqual(result.at(0)[0], 0.5)
        self.assertEqual(result.at(1)[0], 1.0)
        self.assertEqual(result.at(2)[0], 1.5)
        self.assertEqual(result.size(), 3)

    def test_Field_generic_binary_op(self):
        def generic_binary_op(v1, v2):
            return v1 * 2.0 + v2

        field = infinity.Field("f", [1, 2, 3])
        result = infinity.binaryFieldOp("generic op", field, field, generic_binary_op)
        self.assertEqual(result.name(), "generic op")
        self.assertEqual(result.at(0)[0], 3.0)
        self.assertEqual(result.at(1)[0], 6.0)
        self.assertEqual(result.at(2)[0], 9.0)
        self.assertEqual(result.size(), 3)

    def test_Field_generic_scalar_op(self):
        def generic_scalar_op(v):
            return v * 2.0

        field = infinity.Field("f", [1, 2, 3])
        result = infinity.scalarFieldOp("generic op", field, generic_scalar_op)
        self.assertEqual(result.name(), "generic op")
        self.assertEqual(result.at(0)[0], 2.0)
        self.assertEqual(result.at(1)[0], 4.0)
        self.assertEqual(result.at(2)[0], 6.0)
        self.assertEqual(result.size(), 3)

    def test_Interpolation(self):
        comm = infinity.getCommunicator()
        donor = loadGrid("10x10x10_regular.lb8.ugrid")
        receptor = loadGrid("15x15x15_regular.lb8.ugrid")
        interpolator = infinity.Interpolation(
            plugin_path, "RefinePlugins", donor, receptor, comm)

        field = infinity.Field("Squirtle", infinity.NodeAssociation(), [x for x in range(donor.nodeCount())])
        interpolator.interpolate(field)

    def test_FluidSolver(self):
        comm = infinity.getCommunicator()
        mesh = loadGrid("1-cell.lb8.ugrid")
        solver = infinity.PyFluidSolver(
            "HyperSolve", json.dumps({"HyperSolve": hs_config}), mesh, comm)

        solver.setSolutionAtNode(0, [1, 1, 1, 1, 1])

        solver.visualizeVolume("solver_set.vtk")

        density_before = solver.field("density")
        for node_id in range(mesh.nodeCount()):
            if node_id == 0:
                self.assertAlmostEqual(density_before.at(node_id)[0], 1.0)
            else:
                self.assertAlmostEqual(density_before.at(node_id)[0], 1.225)

    def test_MetricCalculator(self):
        comm = infinity.getCommunicator()

        metric_calculator = infinity.MetricCalculator(
            plugin_path, "RefinePlugins")

        mesh = loadGrid("10x10x10_regular.lb8.ugrid")
        density = infinity.Field(
            'density', infinity.NodeAssociation(), [1.0 for x in range(mesh.nodeCount())])

        metric = metric_calculator.calculate(
            mesh, comm, density, json.dumps({"complexity": 100.0}))

        expected = 21.544346900315233
        self.assertAlmostEqual(expected, metric.at(0)[0])

    def test_Snap(self):
        comm = infinity.getCommunicator()
        mesh = loadGrid("1-cell.lb8.ugrid")
        snap = infinity.Snap(mesh, comm)
        field = infinity.Field("Scyther", infinity.NodeAssociation(), [x for x in range(mesh.nodeCount())])
        snap.add(field)

        field_names = snap.listFields()
        self.assertEqual(len(field_names), 1)
        self.assertEqual(field_names[0], "Scyther")

    def test_StructuredMesh(self):
        comm = infinity.getCommunicator()
        grid_filename = path.join(one_ring_root, "grids/plot3d/laura_cube/laura.g")
        structured_mesh = infinity.import_structured_mesh("regularsolve", path.join(grid_filename))
        self.assertEqual([0, 1], structured_mesh.blockIds())
        self.assertEqual([2, 3, 2], structured_mesh.blockNodeDimensions(0))
        self.assertEqual([3, 2, 2], structured_mesh.blockNodeDimensions(1))

        connectivity = infinity.buildStructuredMeshConnectivity(comm, structured_mesh)
        structured_tinf_mesh = infinity.convertStructuredToUnstructuredMesh(comm, structured_mesh, connectivity)
        self.assertEqual(structured_mesh.blockIds(), structured_tinf_mesh.blockIds())
        self.assertEqual(20, structured_tinf_mesh.nodeCount())

    def test_StructuredMeshAdaptation(self):
        comm = infinity.getCommunicator()
        grid_filename = path.join(one_ring_root, "grids/plot3d/laura_cube/laura.g")
        mesh = infinity.import_structured_mesh("regularsolve", path.join(grid_filename))
        connectivity = infinity.buildStructuredMeshConnectivity(comm, mesh)
        mesh = infinity.convertStructuredToUnstructuredMesh(comm, mesh, connectivity)
        mesh_adaptation = infinity.StructuredMeshAdaptationPlugin(plugin_path, "regularsolve")
        options = {"block type": "boundary-layer", "wall spacing": 0.1}
        field = infinity.Field("metric", infinity.NodeAssociation(), [1.0 for node_id in range(mesh.nodeCount())])
        for block_id in mesh.blockIds():
            mesh_adaptation.adapt(block_id, mesh, field, json.dumps(options))
        infinity.syncNodeCoordinates(comm, mesh)
        self.assertEqual(20, mesh.nodeCount())

    def test_StructuredMeshDoubling(self):
        grid_filename = path.join(one_ring_root, "grids/plot3d/laura_cube/laura.g")
        structured_mesh = infinity.import_structured_mesh("regularsolve", path.join(grid_filename))
        structured_mesh.doubleBlock(0, 2)
        self.assertEqual([0, 1], structured_mesh.blockIds())
        self.assertEqual([2, 3, 3], structured_mesh.blockNodeDimensions(0))
        self.assertEqual([3, 2, 2], structured_mesh.blockNodeDimensions(1))

    def test_StructuredMeshFilter(self):
        comm = infinity.getCommunicator()
        grid_filename = path.join(one_ring_root, "grids/plot3d/laura_cube/laura.g")
        structured_mesh = infinity.import_structured_mesh("regularsolve", path.join(grid_filename))
        block_sides_to_filter = {0: {infinity.IMIN}}
        block_side_filter = infinity.BlockSideFilter(comm, structured_mesh, block_sides_to_filter)
        filtered_mesh = block_side_filter.getMesh()
        self.assertEqual([0], filtered_mesh.blockIds())
        self.assertEqual([1, 3, 2], filtered_mesh.blockNodeDimensions(0))
        block_id, i, j, k = filtered_mesh.previousBlockIJK(0, 0, 1, 1)
        self.assertEqual(0, block_id)
        self.assertEqual(0, i)
        self.assertEqual(1, j)
        self.assertEqual(1, k)

    def test_MetricManipulator(self):
        comm = infinity.getCommunicator()
        mesh_filename = path.join(one_ring_root, "grids/meshb/cube-01.meshb")
        mesh = infinity.import_mesh("RefinePlugins", mesh_filename)
        implied_metric = infinity.MetricManipulator.calcImpliedMetricAtNodes(comm, mesh)
        expected_complexity = 34.42456184432484
        self.assertAlmostEqual(expected_complexity,
                               infinity.MetricManipulator.calcComplexity(comm, mesh, implied_metric))

        target_complexity = 200.0
        scaled_to_complexity_metric = infinity.MetricManipulator.scaleToTargetComplexity(comm, mesh, implied_metric,
                                                                                         target_complexity)
        self.assertAlmostEqual(target_complexity,
                               infinity.MetricManipulator.calcComplexity(comm, mesh, scaled_to_complexity_metric))

        intersected_metric = infinity.MetricManipulator.intersect(implied_metric, scaled_to_complexity_metric)
        self.assertNotAlmostEqual(target_complexity,
                                  infinity.MetricManipulator.calcComplexity(comm, mesh, intersected_metric))

        intersected_metric = infinity.MetricManipulator.scaleToTargetComplexity(comm, mesh, intersected_metric,
                                                                                target_complexity)
        self.assertAlmostEqual(target_complexity,
                               infinity.MetricManipulator.calcComplexity(comm, mesh, intersected_metric))

        target_spacing = 0.5
        metric_with_prescribed_spacing = infinity.MetricManipulator.setPrescribedSpacingOnMetric(comm, mesh,
                                                                                                 implied_metric, {1},
                                                                                                 target_complexity,
                                                                                                 target_spacing)
        self.assertAlmostEqual(target_complexity,
                               infinity.MetricManipulator.calcComplexity(comm, mesh, metric_with_prescribed_spacing))

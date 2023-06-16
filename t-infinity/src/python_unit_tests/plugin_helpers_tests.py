from unittest import TestCase
from os import path
import infinity_plugins as infinity


class TestPluginHelpers(TestCase):
    def test_get_visualization_plugin(self):
        infinity.get_visualization_plugin("ParfaitViz")
        infinity.get_visualization_plugin("RefinePlugins")
        infinity.get_visualization_plugin("ParfaitViz")

    def test_import_mesh(self):
        test_directory = path.dirname(path.realpath(__file__))
        grid_path = path.realpath(
            path.join(test_directory, "../../../grids/lb8.ugrid/1-cell.lb8.ugrid"))
        mesh = infinity.import_mesh("RefinePlugins", grid_path)
        self.assertEqual(mesh.nodeCount(), 8)
        self.assertEqual(mesh.cellCount(), 1)

    def test_field_norm(self):
        field = infinity.Field("pikachu", [1, 2, 3])
        l1_norm = infinity.calcLPNorm(field, lambda x: x < 2, lambda x: x+1, 1)
        self.assertEqual(l1_norm, 4.0)
        l2_norm = infinity.calcLPNorm(field, lambda x: x < 2, lambda x: x+1, 2)
        self.assertEqual(l2_norm, 6**0.5)

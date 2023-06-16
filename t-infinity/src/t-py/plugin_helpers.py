import os
from . import _infinity_plugins as infinity


def get_plugin_library_path():
    script_directory = os.path.dirname(os.path.realpath(__file__))
    lib_directory = os.path.join(script_directory, "../../lib")
    return os.path.realpath(lib_directory)


def import_mesh(preprocessor_name, mesh_filename, comm=infinity.getCommunicator()):
    preprocessor = infinity.PyMeshLoader(get_plugin_library_path(), preprocessor_name)
    return preprocessor.load(mesh_filename, comm)


def import_structured_mesh(preprocessor_name, mesh_filename, comm=infinity.getCommunicator()):
    preprocessor = infinity.StructuredMeshLoader(get_plugin_library_path(), preprocessor_name)
    return preprocessor.load(mesh_filename, comm)


def write_mesh(plugin_name, output_filename, mesh, comm=infinity.getCommunicator()):
    infinity.PyViz(output_filename, mesh, comm, get_plugin_library_path(), plugin_name).visualize()


def import_metric_calculator(calculator_name="RefinePlugins"):
    calculator = infinity.MetricCalculator(get_plugin_library_path(), calculator_name)
    return calculator


def import_interpolator(interpolator_name, donor, receptor, comm):
    return infinity.Interpolation(get_plugin_library_path(), interpolator_name, donor, receptor, comm)


def get_mesh_size(mesh, comm):
    nnodes = 0
    for node_id in range(mesh.nodeCount()):
        if mesh.nodeOwner(node_id) == mesh.partitionId():
            nnodes += 1
    return comm.sum(nnodes)


def get_visualization_plugin(viz_plugin_name):
    return infinity.VizPlugin(get_plugin_library_path(), viz_plugin_name)


def calcLPNorm(field, do_own, parallel_sum, p):
    lp_norm = 0.0
    for i in range(field.size()):
        if do_own(i):
            for v in field.at(i):
                lp_norm += v ** p
    lp_norm = parallel_sum(lp_norm) ** (1.0 / p)
    return lp_norm

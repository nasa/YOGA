import infinity_plugins as infinity
import math


def test_function(x,y,z):
    return 10.0*x*x + 10.0*y*y + 2.0*z*z + math.tanh(10.0*(z-0.5))


def createScalarTestField(mesh):
    scalar_field = []
    for i in range(mesh.nodeCount()):
        x,y,z = mesh.nodeCoordinate(i)
        scalar_field.append(test_function(x,y,z))
    return scalar_field


infinity.initializeMPI()
comm = infinity.getCommunicator()
refine_dir = "../../../install/lib"
refine_name = "RefinePlugins"
geometry_meshb='../../../grids/meshb/cube-cylinder-01.meshb'
mesh = infinity.import_mesh(refine_name, geometry_meshb, comm)
mesh_adaptation_plugin = infinity.MeshAdaptationPlugin(refine_dir, refine_name)
metric_calculator = infinity.MetricCalculator(refine_dir, refine_name)

for i in range(5):
    output_project="mesh-adapt-%02d" % i
    json_options = '{"geometry_meshb":"%s","output_project":"%s","sweeps":2}' % (geometry_meshb,output_project)
    scalar_field = createScalarTestField(mesh)
    py_scalar_field = infinity.getFieldFromScalarArray("test", scalar_field)
    metric = metric_calculator.calculate(mesh, comm, py_scalar_field, '{"complexity":1000}')
    mesh = mesh_adaptation_plugin.adapt(mesh,comm,metric,json_options)
    geometry_meshb="%s.meshb" % output_project

infinity.finalizeMPI()

import infinity_plugins as infinity
import sys
import json

def complexity(i):
    return (1000.0)

with open(sys.argv[1]) as f:
    config = json.load(f)

infinity.initializeMPI()
refine_dir = config["refine"]["directory"]
refine_name = config["refine"]["name"]
hs_dir = config["HyperSolve"]["directory"]
hs_name = config["HyperSolve"]["name"]
hs_config = json.dumps(config["HyperSolve"])
geometry_meshb='../../../grids/meshb/om6-tiny-01.meshb'

comm = infinity.getCommunicator()
mesh = infinity.getMeshFromPreProcessor(comm, geometry_meshb, refine_dir, refine_name)
mesh_adaptation_plugin = infinity.MeshAdaptationPlugin(refine_dir, refine_name)
metric_calculator = infinity.getMetricCalculator(refine_dir, refine_name)

for i in range(0,2):
    output_project="om6-tmr-%02d" % (i+2)
    fluid_solver = infinity.FluidSolver(mesh,comm,hs_config,hs_dir,hs_name)
    if(i>0):
      for j in range(mesh.nodeCount()):
        fluid_solver.setSolutionAtNode(j, interpolated_solution.at(j))

    fluid_solver.solve("")
    solution_variable_names = fluid_solver.expectsSolutionAs()
    solution = fluid_solver.extractScalarsAsField("Q",solution_variable_names)
    mach = fluid_solver.field("mach")

    opts = "{\"complexity\":%d}" % complexity(i)
    metric = metric_calculator.calculate(mesh, comm, mach, opts )
    # use default segments_per_radian for larger complexities, 1 is for fast MR
    json_options = '{"geometry_meshb":"%s","output_project":"%s","segments_per_radian":1.0}' % (geometry_meshb,output_project)
    new_mesh = mesh_adaptation_plugin.adapt(mesh,comm,metric,json_options)
    interpolator = infinity.getInterpolator(mesh, new_mesh, comm, refine_dir, refine_name)
    interpolated_solution = interpolator.interpolate(solution)
    fluid_solver.unloadPlugin()
    mesh = new_mesh
    geometry_meshb="%s.meshb" % output_project

infinity.finalizeMPI()

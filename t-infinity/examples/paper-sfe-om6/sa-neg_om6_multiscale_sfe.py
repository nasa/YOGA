import infinity_plugins as infinity
import sys
import json

def complexity(i):
    number_of_fixed_complexity_iterations =  3
    initial_complexity = 50000.0
    step = i // number_of_fixed_complexity_iterations
    return (initial_complexity * 2.0 ** step)

def visualize(viz_plugin, solver, mesh, project):
    density = solver.extractScalarsAsField("density",["density"])
    x_velocity = solver.extractScalarsAsField("u",["u"])
    y_velocity = solver.extractScalarsAsField("v",["v"])
    z_velocity = solver.extractScalarsAsField("w",["w"])
    pressure = solver.extractScalarsAsField("pressure",["pressure"])

    surface_filter = infinity.SurfaceTagFilter(mesh,[2,3,4,5,6,7,8,9,10,11,12])
    surface_mesh = surface_filter.getMesh()
    viz = viz_plugin.createViz(project + "-surface",surface_mesh,comm)
    viz.add(surface_filter.apply(density))
    viz.add(surface_filter.apply(x_velocity))
    viz.add(surface_filter.apply(y_velocity))
    viz.add(surface_filter.apply(z_velocity))
    viz.add(surface_filter.apply(pressure))
    viz.visualize()

with open(sys.argv[1]) as f:
    config = json.load(f)
infinity.initializeMPI()
refine_dir = config["refine"]["directory"]
refine_name = config["refine"]["name"]
sfe_dir = config["sfe"]["directory"]
sfe_name = config["sfe"]["name"]
sfe_config = json.dumps(config["sfe"])

geometry_meshb='../../../grids/meshb/om6-tmr-01.meshb'
comm = infinity.getCommunicator()
mesh = infinity.import_mesh(refine_name, geometry_meshb, comm)
mesh_adaptation_plugin = infinity.MeshAdaptationPlugin(refine_dir, refine_name)
metric_calculator = infinity.getMetricCalculator(refine_dir, refine_name)

viz_directory = config["visualization plugin"]["directory"]
viz_name = config["visualization plugin"]["name"]
viz_plugin = infinity.getVizPlugin(viz_directory, viz_name)

for i in range(60):
    input_project="om6-tmr-%02d" % (i+1)
    output_project="om6-tmr-%02d" % (i+2)
    status = "case step %s to %s" % (input_project,output_project)
    print(status)
    fluid_solver = infinity.FluidSolver(mesh,comm,sfe_config,sfe_dir,sfe_name)
    if(i>0):
      for j in range(mesh.nodeCount()):
        fluid_solver.setSolutionAtNode(j, interpolated_solution.at(j))

    #infinity.setSolutionFromSnap(input_project + ".snap", mesh, fluid_solver, comm)
    infinity.writeSolutionToSnap(input_project + "-init.snap", mesh, fluid_solver, comm)
    fluid_solver.solve("")
    infinity.writeSolutionToSnap(input_project + ".snap", mesh, fluid_solver, comm)
    visualize(viz_plugin, fluid_solver, mesh, input_project)
    solution_variable_names = fluid_solver.expectsSolutionAs()
    solution = fluid_solver.extractScalarsAsField("Q",solution_variable_names)
    mach = fluid_solver.field("mach")

    opts = "{\"complexity\":%d}" % complexity(i)
    metric = metric_calculator.calculate(mesh, comm, mach, opts)
    json_options = "{\"geometry_meshb\":\"%s\",\"output_project\":\"%s\",\"sweeps\":10}" % (geometry_meshb,output_project)
    new_mesh = mesh_adaptation_plugin.adapt(mesh,comm,metric,json_options)
    interpolator = infinity.getInterpolator(mesh, new_mesh, comm,refine_dir,refine_name)
    interpolated_solution = interpolator.interpolate(solution)
    fluid_solver.unloadPlugin()
    mesh = new_mesh
    geometry_meshb="%s.meshb" % output_project

infinity.finalizeMPI()

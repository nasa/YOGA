import infinity_plugins as infinity
import sys


def visualize(solver, mesh_in, comm_in, project):
    density = solver.field("density")
    u = solver.field("u")
    v = solver.field("v")
    w = solver.field("w")
    pressure = solver.field("pressure")
    mach = solver.field("mach")
    update = solver.field("update")
    turb = solver.field("turbulence variable")

    viz_creator = infinity.getVizPlugin()
    viz = viz_creator.createViz("full-volume",'{}',project,mesh_in,comm_in)
    viz.add(density)
    viz.add(u)
    viz.add(v)
    viz.add(w)
    viz.add(pressure)
    viz.add(mach)
    viz.add(update)
    viz.add(turb)
    viz.visualize()

def complexity(i):
    number_of_fixed_complexity_iterations = 5
    initial_complexity = 32000.0
    step = i // number_of_fixed_complexity_iterations
    return (initial_complexity * 2.0 ** step)

infinity.initializeMPI()

comm = infinity.getDomainCommunicator()
mesh = infinity.getDomainMesh()
mesh_adaptation_plugin = infinity.MeshAdaptationPlugin()
metric_calculator = infinity.getMetricCalculator()

geometry_meshb='../../../grids/meshb/om6-tmr-01.meshb'
for i in range(0,50):
    output_project="om6-tmr-%02d" % (i+2)
    fluid_solver = infinity.FluidSolver(mesh,comm)
    if(i>0):
      for j in range(mesh.nodeCount()):
        fluid_solver.setSolutionAtNode(j, interpolated_solution.at(j))

    fluid_solver.solve("")
    solution_variable_names = fluid_solver.expectsSolutionAs()
    solution = fluid_solver.extractScalarsAsField("Q",solution_variable_names)
    #visualize(fluid_solver, mesh, comm, output_project)
    mach = fluid_solver.field("mach")

    opts = "{\"complexity\":%d}" % complexity(i)
    metric = metric_calculator.calculate(mesh, comm, mach, opts )
    json_options = "{\"geometry_meshb\":\"%s\",\"output_project\":\"%s\",\"sweeps\":10}" % (geometry_meshb,output_project)
    new_mesh = mesh_adaptation_plugin.adapt(mesh,comm,metric,json_options)
    interpolator = infinity.getInterpolator(mesh, new_mesh, comm)
    interpolated_solution = interpolator.interpolate(solution)
    fluid_solver.unloadPlugin()
    mesh = new_mesh
    geometry_meshb="%s.meshb" % output_project


infinity.finalizeMPI()

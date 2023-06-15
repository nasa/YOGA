import infinity_plugins as infinity
import sys
import json

with open(sys.argv[1]) as f:
    config = json.load(f)

infinity.initializeMPI()

adaptation_options = config["adaptation options"]

comm = infinity.getCommunicator()
mesh = infinity.import_mesh("RefinePlugins", adaptation_options["geometry_meshb"], comm)

mesh_adapter = infinity.RefineMeshAdapter(comm)
for i in range(2):
    fluid_solver = infinity.PyFluidSolver(config["fluid solver"], json.dumps(config["HyperSolve"]), mesh, comm)

    if i > 0:
        for j in range(mesh.nodeCount()):
            fluid_solver.setSolutionAtNode(j, interpolated_solution.at(j))

    fluid_solver.solve("")
    solution_variable_names = fluid_solver.expectsSolutionAs()
    solution = infinity.extractScalarsAsNodeField(fluid_solver, mesh, "Q", solution_variable_names)

    adaptation_options["output_project"] = "%s-%02d" % (config["project name"], i + 2)
    adaptation_options["sweeps"] = 2 # run fast, incomplete metric conformity
    new_mesh = mesh_adapter.adaptToComplexity(mesh, fluid_solver.field("mach"), 1000, json.dumps(adaptation_options))
    adaptation_options["geometry_meshb"] = adaptation_options["output_project"] + ".meshb"

    interpolated_solution = mesh_adapter.interpolateField(mesh, new_mesh, solution)
    mesh = new_mesh

infinity.finalizeMPI()

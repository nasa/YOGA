import infinity_plugins as infinity
import sys
import json


def getPrimalDualField(num_equations):
    fields = []
    for root in ["flux-x", "flux-y", "flux-z", "lambda"]:
        for eqn in range(num_equations):
            fields.append("%s-%d" % (root, eqn))
    return fields


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
    solution = infinity.extractScalarsAsNodeField(fluid_solver, mesh, "Q", fluid_solver.expectsSolutionAs())
    primal_dual = infinity.extractScalarsAsNodeField(fluid_solver, mesh, "primal_dual", getPrimalDualField(5))

    adaptation_options["output_project"] = "%s-%02d" % (config["project name"], i + 2)
    adaptation_options["sweeps"] = 2 # run fast, incomplete metric conformity
    new_mesh = mesh_adapter.adaptToComplexity(mesh, primal_dual, 1000, json.dumps(adaptation_options))
    adaptation_options["geometry_meshb"] = adaptation_options["output_project"] + ".meshb"

    interpolated_solution = mesh_adapter.interpolateField(mesh, new_mesh, solution)
    mesh = new_mesh

infinity.finalizeMPI()

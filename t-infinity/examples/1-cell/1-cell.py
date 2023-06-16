import infinity_plugins as infinity
import sys
import json

infinity.initializeMPI()

with open(sys.argv[1]) as f:
    config = json.load(f)

comm = infinity.getCommunicator()
mesh = infinity.import_mesh(config["preprocessor"], config["mesh filename"], comm)

solver = infinity.PyFluidSolver("HyperSolve", json.dumps(config), mesh, comm)

solver.solve("")

infinity.finalizeMPI()

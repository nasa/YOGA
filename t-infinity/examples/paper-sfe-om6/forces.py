import infinity_plugins as infinity
import sys
import json
import math

def getViscosity(temperature):
    sutherland_const1 = 0.1458205E-05
    sutherland_const2 = 110.333333
    return sutherland_const1 * math.sqrt(temperature) * temperature / (temperature + sutherland_const2);
runtime = infinity.RunTime()

comm = runtime.getCommunicator()
mesh_name = sys.argv[1]
refine_dir = "../../../install/plugin-refine/lib"
refine_name = "RefinePlugins"
mesh = runtime.getMeshFromPreProcessor(comm, mesh_name, refine_dir, refine_name)

fluid_solve_dir = "../../../install/hypersolve/lib"
fluid_solve_name = "HyperSolve"
with open("hypersolve.json") as f:
    config = json.load(f)
hs_config = json.dumps(config["HyperSolve"])
fluid_solver = runtime.getFluidSolver(mesh, comm, hs_config, fluid_solve_dir, fluid_solve_name)

snap_name = sys.argv[2]

snap = infinity.Snap(snap_name, mesh, comm)
density = snap.field("density")
u = snap.field("u")
v = snap.field("v")
w = snap.field("w")
pressure = snap.field("pressure")
sfe_sa_neg = snap.field("sfe-turb-variable")

temperature_ref = config["HyperSolve"]["states"]["farfield"]["temperature"]
density_ref = config["HyperSolve"]["states"]["farfield"]["density"]
viscosity_ref = getViscosity(temperature_ref)

turb_data = []
for i in range(mesh.nodeCount()):
    turb_data.append(sfe_sa_neg.at(i)[0] * viscosity_ref / density_ref)
hs_sa_neg = infinity.Field("hypersolve-sa-var", turb_data)

new_snap = infinity.Snap(mesh, comm)
new_snap.addField(density)
new_snap.addField(u)
new_snap.addField(v)
new_snap.addField(w)
new_snap.addField(pressure)
new_snap.addField(hs_sa_neg)
new_snap.writeFile("new.snap")

runtime.setSolutionFromSnap("new.snap", mesh, fluid_solver, comm)

fluid_solver.solve("")

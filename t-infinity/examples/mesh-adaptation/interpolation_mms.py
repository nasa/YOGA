import infinity_plugins as infinity

import math
import sys
import json

def hyperMMS(comm, mesh, config):
    hs_config = json.dumps(config["HyperSolve"])
    solver = infinity.FluidSolver(mesh, comm, hs_config, "../../../install/lib", "HyperSolve")
    solver.solve("")
    return solver.field("exact temperature")

with open("interpolation_mms.json", "r") as f:
    config = json.load(f)

infinity.initializeMPI()
comm = infinity.getCommunicator()
refine_dir = "../../../install/lib"
refine_name = "RefinePlugins"
geometry_meshb='../../../grids/meshb/cube-cylinder-01.meshb'
mesh = infinity.getMeshFromPreProcessor(comm, geometry_meshb, refine_dir, refine_name)
mesh_adaptation_plugin = infinity.MeshAdaptationPlugin(refine_dir, refine_name)
metric_calculator = infinity.getMetricCalculator(refine_dir, refine_name)

for i in range(5):
    output_project="mesh-adapt-%02d" % i
    json_options = '{"geometry_meshb":"%s","output_project":"%s"}' % (geometry_meshb,output_project)
    scalar_field = hyperMMS(comm, mesh, config)
    metric = metric_calculator.calculate(mesh, comm, scalar_field, '{"complexity":500}')
    mesh = mesh_adaptation_plugin.adapt(mesh,comm,metric,json_options)
    geometry_meshb="%s.meshb" % output_project

infinity.finalizeMPI()

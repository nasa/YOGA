import infinity_plugins as infinity
import sys
import os
import json

infinity.initializeMPI()
global_comm = infinity.getCommunicator()

weights = [1,1]
my_domain_id = infinity.assignColorByWeight(global_comm, weights)
comm = infinity.splitCommunicator(global_comm, my_domain_id)

preprocessor = "NC_PreProcessor"
grid_file = ""
if(my_domain_id == 0):
    grid_file = "../../../grids/lb8.ugrid/wing.lb8.ugrid"
elif(my_domain_id == 1):
    grid_file = "../../../grids/lb8.ugrid/store.lb8.ugrid"

mesh = infinity.import_mesh(preprocessor, grid_file, comm)

composite = infinity.createCompositeMesh(mesh,my_domain_id,comm,global_comm)

infinity.exportUgrid("composite_mesh",composite,global_comm)

#viz_plugin = infinity.get_visualization_plugin("ParfaitViz")
#viz = viz_plugin.createViz("dog", composite, global_comm)
#viz.visualize()

infinity.finalizeMPI()


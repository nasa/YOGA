import infinity_plugins as infinity
import sys
import os
import json

infinity.initializeMPI()
with open(sys.argv[1]) as f:
    bc_info_string = f.read()

global_comm = infinity.getCommunicator()

weights = [8414197,2645804,2645804,2645804,2645804]
domain_names = ["fuselage","blade-1","blade-2","blade-3","blade-4"]
mesh_filenames = ["/u/cdruyor/bob-grads/heli.lb8.ugrid",
 "/u/cdruyor/bob-grads/uh60_alw_blade_tab_bl2_t2_ft.lb8.ugrid",
 "/u/cdruyor/bob-grads/uh60_alw_blade_tab_bl2_t2_ft.lb8.ugrid",
 "/u/cdruyor/bob-grads/uh60_alw_blade_tab_bl2_t2_ft.lb8.ugrid",
 "/u/cdruyor/bob-grads/uh60_alw_blade_tab_bl2_t2_ft.lb8.ugrid"]

my_domain_id = infinity.assignColorByWeight(global_comm, weights)
comm = infinity.splitCommunicator(global_comm, my_domain_id)
domain_name = domain_names[my_domain_id]
mesh_filename = mesh_filenames[my_domain_id]

yoga_directory = infinity.get_plugin_library_path()
yoga_name = "YogaPlugin"

mesh = infinity.import_mesh("NC_PreProcessor", mesh_filename, comm)

blade_set_offset = [2.8430E+01, 0.0000E+00, 2.6250E+01]
axis_begin = [2.8430E+01, 0.0, 0.0]
axis_end = [2.8430E+01, 0.0, 2.6250E+01]
if(domain_name == "blade-1"):
    motion = infinity.Motion()
    motion.addTranslation(blade_set_offset)
    mesh = infinity.moveMesh(mesh,motion)
elif(domain_name == "blade-2"):
    motion = infinity.Motion()
    motion.addTranslation(blade_set_offset)
    motion.addRotation(axis_begin,axis_end,90.0)
    mesh = infinity.moveMesh(mesh,motion)
elif(domain_name == "blade-3"):
    motion = infinity.Motion()
    motion.addTranslation(blade_set_offset)
    motion.addRotation(axis_begin,axis_end,180.0)
    mesh = infinity.moveMesh(mesh,motion)
elif(domain_name == "blade-4"):
    motion = infinity.Motion()
    motion.addTranslation(blade_set_offset)
    motion.addRotation(axis_begin,axis_end,270.0)
    mesh = infinity.moveMesh(mesh,motion)

domain_assembler = infinity.DomainAssembler(mesh,global_comm,my_domain_id,
                                              bc_info_string,
                                              yoga_directory, yoga_name)

#viz_plugin = infinity.getVizPlugin(config["parfait"]["directory"], config["parfait"]["name"])
ids_of_nodes_to_freeze = domain_assembler.performDomainAssembly()

#node_statuses = domain_assembler.field("node statuses")

#viz_plugin = infinity.get_visualization_plugin("ParfaitViz")
#viz = viz_plugin.createViz(domain_name, mesh, comm)
#viz.add(node_statuses)
#viz.visualize()

infinity.finalizeMPI()

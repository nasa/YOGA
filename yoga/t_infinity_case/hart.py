import infinity_plugins as infinity
import sys
import os
import json

infinity.initializeMPI()
with open(sys.argv[1]) as f:
    bc_info_string = f.read()


global_comm = infinity.getCommunicator()
weights = [3455726,900723,900723,900723,900723]
domain_names = ["fuselage",
        "blade-1",
        "blade-2",
        "blade-3",
        "blade-4"]
mesh_filenames = ["../../grids/lb8.ugrid/hart_body_mixed.lb8.ugrid",
            "../../grids/lb8.ugrid/hart_blade_mixed.lb8.ugrid",
            "../../grids/lb8.ugrid/hart_blade_mixed.lb8.ugrid",
            "../../grids/lb8.ugrid/hart_blade_mixed.lb8.ugrid",
            "../../grids/lb8.ugrid/hart_blade_mixed.lb8.ugrid"]

my_domain_id = infinity.assignColorByWeight(global_comm, weights)
comm = infinity.splitCommunicator(global_comm, my_domain_id)
domain_name = domain_names[my_domain_id]
mesh_filename = mesh_filenames[my_domain_id]

yoga_directory = infinity.get_plugin_library_path()
yoga_name = "YogaPlugin"

mesh = infinity.import_mesh("NC_PreProcessor", mesh_filename, comm)

blade_set_offset = [0.7652, 0.0000E+00, 0.796]
axis_begin = [.7652, 0.0, 0.0]
axis_end = [.7652, 0.0, .796]
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

ids_of_nodes_to_freeze = domain_assembler.performDomainAssembly()

node_statuses = domain_assembler.field("node statuses")
boundary_conditions = domain_assembler.field("boundary conditions")
cell_statuses = domain_assembler.field("cell statuses")

#solver.freezeNodes(ids_of_nodes_to_freeze)

viz_plugin = infinity.get_visualization_plugin("ParfaitViz")

lo = [-10, 0, -10]
hi = [10, 0,  10]
clip_filter = infinity.ClipExtentFilter(comm,mesh,lo,hi)
sliced_mesh = clip_filter.getMesh()
sliced_node_statuses = clip_filter.apply(node_statuses)
sliced_cell_statuses = clip_filter.apply(cell_statuses)
viz = viz_plugin.createViz(domain_name+"-y-slice", sliced_mesh, comm)
viz.add(sliced_node_statuses)
viz.add(sliced_cell_statuses)
viz.visualize()


lo = [0.769115, -10, -10]
hi = [0.769115, 10,  10]
clip_filter = infinity.ClipExtentFilter(comm,mesh,lo,hi)
sliced_mesh = clip_filter.getMesh()
sliced_node_statuses = clip_filter.apply(node_statuses)
sliced_cell_statuses = clip_filter.apply(cell_statuses)
viz = viz_plugin.createViz(domain_name+"-x-slice", sliced_mesh, comm)
viz.add(sliced_node_statuses)
viz.add(sliced_cell_statuses)
viz.visualize()

#viz = viz_plugin.createViz(domain_name, mesh, comm)
#viz.add(node_statuses)
#viz.add(cell_statuses)
#viz.visualize()

infinity.finalizeMPI()

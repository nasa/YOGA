import infinity_plugins as infinity
import sys
import json

infinity.initializeMPI()
with open(sys.argv[1]) as f:
    yoga_bc_string = f.read()

global_comm = infinity.getCommunicator()
mesh_filenames = ["wing.lb8.ugrid",
       "store.lb8.ugrid"]
domain_names = ["wing","store"]

weights = [1,1]
my_domain_id = infinity.assignColorByWeight(global_comm, weights)
comm = infinity.splitCommunicator(global_comm, my_domain_id)

mesh_filename = mesh_filenames[my_domain_id]
domain_name = domain_names[my_domain_id]

mesh = infinity.import_mesh("NC_PreProcessor", mesh_filename, comm)

yoga_directory = infinity.get_plugin_library_path()
yoga_name = "YogaPlugin"
domain_assembler = infinity.DomainAssembler(mesh, global_comm, my_domain_id,
                                            yoga_bc_string,
                                            yoga_directory, yoga_name)

ids_of_nodes_to_freeze = domain_assembler.performDomainAssembly()

node_statuses = domain_assembler.field("node statuses")

viz_plugin = infinity.get_visualization_plugin("ParfaitViz")
viz = viz_plugin.createViz(domain_name, mesh, comm)
viz.add(node_statuses)
viz.visualize()



infinity.finalizeMPI()

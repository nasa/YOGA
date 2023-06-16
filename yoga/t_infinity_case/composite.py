import infinity_plugins as infinity
import sys
import json

infinity.initializeMPI()
with open(sys.argv[1]) as f:
    yoga_bc_string = f.read()

comm = infinity.getCommunicator()
mesh_filename = "composite.lb8.ugrid"
domain_name = "composite"

mesh = infinity.import_mesh("NC_PreProcessor", mesh_filename, comm)

yoga_directory = infinity.get_plugin_library_path()
yoga_name = "YogaPlugin"
domain_id = 0
domain_assembler = infinity.DomainAssembler(mesh, comm, domain_id,
                                            yoga_directory, yoga_name,
                                            yoga_bc_string
                                            )


viz_plugin = infinity.get_visualization_plugin("ParfaitViz")
viz = viz_plugin.createViz(domain_name, mesh, comm)
#viz.add(node_statuses)
viz.visualize("dog.vtk",mesh,[],comm,"ParfaitViz")



infinity.finalizeMPI()

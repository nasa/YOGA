import infinity_plugins as infinity
import sys
import json

infinity.initializeMPI()
with open(sys.argv[1]) as f:
    config = json.load(f)

with open(sys.argv[2]) as f:
    yoga_bc_string = f.read()

global_comm = infinity.getCommunicator()
mesh_filenames = ["../../../grids/lb8.ugrid/wing.lb8.ugrid",
                  "../../../grids/lb8.ugrid/store.lb8.ugrid"]
domain_names = ["wing", "store"]

weights = [1, 1]
my_domain_id = infinity.assignColorByWeight(global_comm, weights)
comm = infinity.splitCommunicator(global_comm, my_domain_id)
domain_config = config["domains"][my_domain_id]

mesh_filename = mesh_filenames[my_domain_id]
domain_name = domain_names[my_domain_id]

mesh = infinity.import_mesh("NC_PreProcessor", mesh_filename, comm)

fs_name = config["fluid solver"]
fs_config = json.dumps(domain_config[fs_name])
solver = infinity.PyFluidSolver(fs_name, fs_config, mesh, comm)

yoga_directory = infinity.get_plugin_library_path()
yoga_name = "YogaPlugin"
domain_assembler = infinity.DomainAssembler(mesh, global_comm, my_domain_id,
                                            yoga_directory, yoga_name, yoga_bc_string)

ids_of_nodes_to_freeze = domain_assembler.performDomainAssembly()

solver.freezeNodes(ids_of_nodes_to_freeze)

n = 1
for i in range(n):
    if comm.rank() == 0:
        print("Domain " + domain_name + "Iteration: " + str(i))
    receptor_solutions = domain_assembler.updateReceptorSolutions(solver)
    for node_id in receptor_solutions:
        solver.setSolutionAtNode(node_id, receptor_solutions[node_id])
    solver.solve("")

solver.visualizeVolume(domain_name, viz_plugin_name=config["visualizer"])

infinity.finalizeMPI()

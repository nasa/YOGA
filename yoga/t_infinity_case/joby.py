import infinity_plugins as infinity
import sys
import os
import json

infinity.initializeMPI()
with open(sys.argv[1]) as f:
    config = json.load(f)

global_comm = infinity.getCommunicator()
weights = []
for d in config["yoga"]["domains"]:
    weights.append(d["estimated load balance"])
if global_comm.rank() == 0:
    print("weights are: " + str(weights))

my_domain_id = infinity.assignColorByWeight(global_comm, weights)
comm = infinity.splitCommunicator(global_comm, my_domain_id)
domain_config = config["yoga"]["domains"][my_domain_id]
domain_name = domain_config["name"]
mesh_filename = domain_config["mesh filename"]

yoga_directory = config["yoga"]["directory"]
yoga_name = config["yoga"]["name"]

mesh = infinity.import_mesh(config["preprocessor"], mesh_filename, comm)

left_offset = [0.259901, 17.586, 0.0]
right_offset = [0.5198, 35.172, 0.0]
if(domain_name == "prop-left"):
    motion = infinity.Motion()
    motion.addTranslation(left_offset)
    mesh = infinity.moveMesh(mesh,motion)
elif(domain_name == "prop-right"):
    motion = infinity.Motion()
    motion.addTranslation(right_offset)
    mesh = infinity.moveMesh(mesh,motion)
elif(domain_name == "fairing-left"):
    motion = infinity.Motion()
    motion.addTranslation(left_offset)
    mesh = infinity.moveMesh(mesh,motion)
elif(domain_name == "fairing-right"):
    motion = infinity.Motion()
    motion.addTranslation(right_offset)
    mesh = infinity.moveMesh(mesh,motion)

domain_assembler = infinity.DomainAssembler(mesh,global_comm,my_domain_id,
                                              json.dumps(config["yoga"]["domains"][my_domain_id]),
                                              yoga_directory, yoga_name)

#viz_plugin = infinity.getVizPlugin(config["parfait"]["directory"], config["parfait"]["name"])
ids_of_nodes_to_freeze = domain_assembler.performDomainAssembly()

node_statuses = domain_assembler.field("node statuses")

#solver.freezeNodes(ids_of_nodes_to_freeze)

#viz = viz_plugin.createViz(domain_name,mesh,comm)
#viz.addField(node_statuses)
#viz.visualize()

infinity.finalizeMPI()

import infinity_plugins as infinity
import sys
import json

def splitCompositeIntoComponents():
    boundary_config = """domain background
              domain sphere
              solid 500
              interpolation 600"""
    ngrids = 2

    mesh_filename = "../../../grids/lb8.ugrid/composite_sphere_cube.lb8.ugrid"
    mesh = infinity.import_mesh("NC_PreProcessor",
                                mesh_filename,
                                global_comm)

    yoga_directory = "../../../install/lib"
    yoga_name = "YogaPlugin"
    domain_assembler = infinity.DomainAssembler(mesh, global_comm, 0,
                                                yoga_directory,
                                                yoga_name,
                                                boundary_config)

    grid_ids = domain_assembler.determineGridIdsForNodes()
    grid_id_field = infinity.getFieldFromScalarArray("grid ids",grid_ids)

    for i in range(0,ngrids):
        component_filter = infinity.ValueFilter(global_comm,mesh,i,i,grid_id_field)
        component_mesh = component_filter.getMesh()
        infinity.exportUgrid("component_"+str(i),component_mesh,global_comm)




infinity.initializeMPI()
global_comm = infinity.getCommunicator()

splitCompositeIntoComponents()

ngrids = 2
domain_names = ["background","sphere"]
mesh_filenames = ["component_0.lb8.ugrid","component_1.lb8.ugrid"]
weights = [1, 1]

my_domain_id = infinity.assignColorByWeight(global_comm, weights)
domain_name = domain_names[my_domain_id]
mesh_filename = mesh_filenames[my_domain_id]
comm = infinity.splitCommunicator(global_comm, my_domain_id)

mesh = infinity.import_mesh("NC_PreProcessor",
                            mesh_filename,
                            comm)

nodeOwner = []
partitionId = mesh.partitionId()
for i in range(mesh.nodeCount()):
    nodeOwner.append(mesh.nodeOwner(i))

max_node_ids = [0,0]

my_max_node_id = 0
for i in range(mesh.nodeCount()):
    if(mesh.globalNodeId(i) > my_max_node_id):
        my_max_node_id = mesh.globalNodeId(i)

max_node_ids[my_domain_id] = my_max_node_id

max_node_ids[0] = global_comm.max(max_node_ids[0])
max_node_ids[1] = global_comm.max(max_node_ids[1])
global_comm.barrier()
if(0 == global_comm.rank()):
    expect_nnodes_a = 13396
    expect_nnodes_b = 3576
    nnodes_a = max_node_ids[0]+1
    nnodes_b = max_node_ids[1]+1
    if(expect_nnodes_a != nnodes_a or expect_nnodes_b != nnodes_b):
        print("FAILED: Yoga--> sphere cube regression")
        print("expected node counts in components "+str(expect_nnodes_a)+" and "+str(expect_nnodes_b))
        print("but got "+str(nnodes_a)+" and "+str(nnodes_b))
        exit(1)
    else:
        print("PASSED")


infinity.finalizeMPI()

import infinity_plugins as infinity
import sys

runtime = infinity.RunTime(sys.argv[1])

comm = runtime.getDomainCommunicator()
mesh = runtime.getDomainMesh()
viz_creator = runtime.getVizPlugin()
viz = viz_creator.createViz("full-volume",'{}',"grid",mesh,comm)

node_owner = []
for i in range(mesh.nodeCount()):
    node_owner.append(float(mesh.nodeOwner(i)))

owners = runtime.getFieldFromScalarArray(node_owner, "owner")
viz.add(owners)
viz.visualize()

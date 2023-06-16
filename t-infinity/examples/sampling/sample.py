import infinity_plugins as infinity
import sys
import json

infinity.initializeMPI()
with open(sys.argv[1]) as f:
    config = json.load(f)

comm = infinity.getCommunicator()
mesh = infinity.import_mesh(config["preprocessor"], config["mesh filename"])
viz_plugin = infinity.get_visualization_plugin(config["visualizer"])

v = viz_plugin.createViz("full", mesh, comm)
v.visualize()

radius = 1
center = [0, 0, 0]
sphere_filter = infinity.ClipSphereFilter(comm, mesh, radius, center)
filtered_mesh = sphere_filter.getMesh()

sphere_viz = viz_plugin.createViz("sphere", filtered_mesh, comm)
sphere_viz.visualize()

box_filter = infinity.ClipExtentFilter(comm, mesh, [0, 0, 0], [1, 1, 1])
filtered_mesh = box_filter.getMesh()

v = viz_plugin.createViz("box", filtered_mesh, comm)
v.visualize()

surface_filter = infinity.SurfaceFilter(comm, mesh)
filtered_mesh = surface_filter.getMesh()
v = viz_plugin.createViz("surfaces", filtered_mesh, comm)
v.visualize()

filter = infinity.SurfaceTagFilter(comm, mesh, [1])
filtered_mesh = filter.getMesh()
v = viz_plugin.createViz("surfaces_by_tag", filtered_mesh, comm)
v.visualize()

plane_filter = infinity.PlaneCutFilter(comm, mesh, [.1,0,0], [1,0,0])
filtered_mesh = plane_filter.getMesh()
v = viz_plugin.createViz("plane-cut", filtered_mesh, comm)
v.visualize()

infinity.finalizeMPI()

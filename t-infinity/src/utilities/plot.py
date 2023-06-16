#!/usr/bin/env python
import infinity_plugins as infinity
import argparse
import sys

def get_args():
    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('--mesh', '-m', default='', help='Input mesh', required=True)
    parser.add_argument('--pre-processor', default='', help='Specify which pre-processor plugin to use for mesh reading.')
    parser.add_argument('--viz-plugin', dest="viz_plugin", default='ParfaitViz', help='Specify which visualization plugin to use for plotting.')
    parser.add_argument('--volume', '-v',default=False, dest="volume", action="store_true", help='Enable volume only plotting')
    parser.add_argument('--output-filename', '-o',default="out", dest="output_name", help='output filename')
    parser.add_argument('--surface', '-s', dest="surface", action="store_true", help='Enable surface plotting')
    parser.add_argument('--no-surface', dest="surface", action="store_false", help='Disable surface plotting, this is default')
    parser.add_argument('--no-volume', dest="volume", action="store_false", help='Disable volume plotting')
    parser.add_argument('--snap', default="", help='Plot fields from snap database.')
    parser.add_argument('--clean', dest="clean", action="store_true", help="Don't automatically add cell tags and owner")
    parser.add_argument('--explode', dest="explode", action="store_true", help='Explode a mesh to see each cell individually')
    parser.add_argument('--x-plane', dest="x_plane", default=""),
    parser.add_argument('--y-plane', dest="y_plane", default=""),
    parser.add_argument('--z-plane', dest="z_plane", default=""),
    parser.set_defaults(volume=True)
    parser.set_defaults(surface=False)
    args = parser.parse_args()
    return args


def root_print(comm, message):
    if comm.rank == 0:
        print(message)

def exit(comm):
    comm.barrier()
    infinity.finalizeMPI()
    sys.exit()

def get_default_pre_processor(filename):
    extension = filename.split(".")[-1]
    if extension == "meshb":
        return "RefinePlugins"
    return "NC_PreProcessor"

def load_mesh(comm, args):
    mesh_filename = args.mesh
    if not mesh_filename:
        args.print_usage()
        sys.exit(1)

    pp = args.pre_processor
    if pp == "":
        pp = get_default_pre_processor(mesh_filename)

    if comm.rank() == 0:
        print("Loading mesh: " + mesh_filename)
    mesh = infinity.import_mesh(pp, mesh_filename, comm)
    return mesh


if __name__ == "__main__":
    infinity.initializeMPI()
    args = get_args()

    comm = infinity.getCommunicator()
    mesh = load_mesh(comm, args)

    plugin_dir = infinity.get_plugin_library_path()

    cell_tags = []
    for c in range(mesh.cellCount()):
        tag = mesh.cellTag(c)
        cell_tags.append(tag)

    cell_owner = []
    for c in range(mesh.cellCount()):
        owner = mesh.cellOwner(c)
        cell_owner.append(owner)


    if args.explode:
        mesh = infinity.explodeMesh(mesh, 0.7)


    fields = []
    if not args.clean:
        fields.append(infinity.Field("cell_tag",infinity.CellAssociation(),cell_tags))
        fields.append(infinity.Field("cell_owner",infinity.CellAssociation(),cell_owner))
    if args.snap:
        root_print(comm, "Reading snap database: " + args.snap)
        snap = infinity.Snap(args.snap, mesh, comm)
        for f in snap.listFields():
            fields.append(snap.field(f))
            root_print(comm, "Loaded field: " + f)

    if args.volume:
        volume_filter = infinity.VolumeFilter(comm,mesh)
        filtered_mesh = volume_filter.getMesh()
        filtered_fields = []
        for f in fields:
            filtered_fields.append(volume_filter.apply(f))
        viz = infinity.PyViz(args.output_name, filtered_mesh,comm,plugin_dir,args.viz_plugin)
        for f in filtered_fields:
            viz.add(f)
        viz.visualize()

    if args.surface:
        surface_filter = infinity.SurfaceFilter(comm,mesh)
        filtered_mesh = surface_filter.getMesh()
        surface_fields = []
        for f in fields:
            surface_fields.append(surface_filter.apply(f))
        viz = infinity.PyViz("surface", filtered_mesh,comm,plugin_dir,args.viz_plugin)
        for f in surface_fields:
            viz.add(f)
        viz.visualize()

    if args.x_plane != "":
        x_location = float(args.x_plane)
        plane_filter = infinity.ClipExtentFilter(comm, mesh, [x_location - 1.0e-6, -1.e20, -1.e20], [x_location + 1.0e-6,1.e20,1.e20])
        filtered_mesh = plane_filter.getMesh()
        filtered_fields = []
        for f in fields:
            filtered_fields.append(plane_filter.apply(f))
        viz = infinity.PyViz(args.output_name, filtered_mesh,comm,plugin_dir,args.viz_plugin)
        for f in filtered_fields:
            viz.add(f)
        viz.visualize()

    if args.y_plane != "":
        y_location = float(args.y_plane)
        plane_filter = infinity.ClipExtentFilter(comm, mesh, [-1.0e20, y_location - 1.0e-6, -1.e20], [1.0e20, y_location + 1.0e-6,1.e20])
        filtered_mesh = plane_filter.getMesh()
        filtered_fields = []
        for f in fields:
            filtered_fields.append(plane_filter.apply(f))
        viz = infinity.PyViz(args.output_name, filtered_mesh,comm,plugin_dir,args.viz_plugin)
        for f in filtered_fields:
            viz.add(f)
        viz.visualize()

    if args.z_plane != "":
        z_location = float(args.z_plane)
        plane_filter = infinity.ClipExtentFilter(comm, mesh, [-1.0e20, -1.0e20, z_location - 1.0e-6], [1.0e20, 1.0e20, z_location + 1.0e-6])
        filtered_mesh = plane_filter.getMesh()
        filtered_fields = []
        for f in fields:
            filtered_fields.append(plane_filter.apply(f))
        viz = infinity.PyViz(args.output_name, filtered_mesh,comm,plugin_dir,args.viz_plugin)
        for f in filtered_fields:
            viz.add(f)
        viz.visualize()


    exit(comm)

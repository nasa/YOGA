#!/usr/bin/env python
import infinity_plugins as infinity
import argparse
import sys

def get_args():
    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('--donor-mesh', '-d', default='', dest="donor_mesh", help='Donor Mesh', required=True)
    parser.add_argument('--recv-mesh', '-r', default='', dest="recv_mesh", help='Receptor Mesh', required=True)
    parser.add_argument('--output-snap', '-o',default="out", dest="recv_snap", help='Output snap filename')
    parser.add_argument('--donor-snap', '-s', default="", dest="donor_snap", help='Donor snap database', required=True)
    args = parser.parse_args()
    return args


def root_print(comm, message):
    if comm.rank() == 0:
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

def load_mesh(comm, mesh_name, pre_processor=""):

    pp = ""
    if pre_processor == "":
        pp = get_default_pre_processor(mesh_name)

    if comm.rank() == 0:
        print("Loading mesh: " + mesh_name)
    return infinity.import_mesh(pp, mesh_name, comm)


if __name__ == "__main__":
    infinity.initializeMPI()
    args = get_args()

    comm = infinity.getCommunicator()
    donor_mesh = load_mesh(comm, args.donor_mesh)
    recv_mesh = load_mesh(comm, args.recv_mesh)

    root_print(comm, "Reading snap database: " + args.donor_snap)
    donor_snap = infinity.Snap(args.donor_snap, donor_mesh, comm)
    interpolator = infinity.Interpolation(infinity.get_plugin_library_path(), "RefinePlugins", donor_mesh, recv_mesh, comm)

    recv_snap = infinity.Snap(recv_mesh, comm)
    fields = []
    for f_name in donor_snap.listFields():
        f_orig = donor_snap.field(f_name)
        root_print(comm, "Loaded field: " + f_name)
        f = f_orig
        if f_orig.association() == infinity.CellAssociation():
            f = f.averageToNodes(donor_mesh)
        f_interpolated = interpolator.interpolate(f)
        if f_orig.association() == infinity.CellAssociation():
            f_interpolated = f_interpolated.averageToCells(recv_mesh)
        recv_snap.add(f_interpolated)

    recv_snap.write(args.recv_snap)
    exit(comm)

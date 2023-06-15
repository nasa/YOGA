#!/usr/bin/env python
import infinity_plugins as infinity
import argparse
import sys

def get_args():
    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('--mesh', '-m', default='', help='Input mesh', required=True)
    parser.add_argument('--pre-processor', default='NC_PreProcessor', help='Specify which pre-processor plugin to use for mesh reading.')
    parser.add_argument('--viz-plugin', default='ParfaitViz', help='Specify which visualization plugin to use for plotting.')
    parser.add_argument('--volume', '-v',dest="volume", action="store_true", help='Enable volume plotting, this is default.')
    parser.add_argument('--output-filename', '-o',default="out", dest="output_name", help='output filename')
    parser.add_argument('--field', '-f',default="Mach Number", dest="field_to_hess", help='Field that Hessian will be calculated')
    parser.add_argument('--snap', default="", help='Plot fields from snap database.', required=True)
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

if __name__ == "__main__":
    infinity.initializeMPI()
    args = get_args()

    mesh_filename = args.mesh
    if not mesh_filename:
        args.print_usage()
        sys.exit(1)

    comm = infinity.getCommunicator()
    if comm.rank() == 0:
        print("Loading mesh: " + mesh_filename)
    mesh = infinity.import_mesh(args.pre_processor, mesh_filename, comm)

    fields = []
    root_print(comm, "Reading snap database: " + args.snap)
    snap = infinity.Snap(args.snap, mesh, comm)

    calculator = infinity.import_metric_calculator()
    f_to_hess = snap.field(args.field_to_hess)
    metric = calculator.calculate(mesh, comm, f_to_hess, "{}")
    fields.append(calculator.getField("hessian"))
    for f in snap.listFields():
        fields.append(snap.field(f))
    infinity.visualize(args.output_name, mesh, fields, comm)
    exit(comm)

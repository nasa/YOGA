#!/usr/bin/env python
import infinity_plugins as infinity
import argparse
import sys
import os

def get_args():
    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('--mesh', '-m', default='', help='Input mesh', required=True)
    parser.add_argument('--pre-processor', default='NC_PreProcessor', help='Specify which pre-processor plugin to use for mesh reading.')
    parser.add_argument('--output-filename', '-o',default="out", dest="output_name", help='output filename')
    parser.add_argument('--snap', default="", help='Plot fields from snap database.', required=False)
    parser.add_argument('--list', '-l', default="", action="store_true", help='List the fields in the database.')
    parser.add_argument('--at', default="", dest="at_association", help='Convert fields to association node or cell.')
    parser.add_argument('--extract-snap', dest="extract_snap", default="", help='What field to write in output snap file')
    parser.add_argument('--create-uniform', dest="create_uniform", default=False, action="store_true", help='Create a uniform scalar field')
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


def add_extension_if_missing(filename, extension):
    e = os.path.splitext(filename)[1]
    e = e.split(".")[-1]
    if e != extension:
        filename += "." + extension
    return filename


def to_nodes(mesh, field):
    f = field
    if field.association() == infinity.CellAssociation():
        f = f.averageToNodes(mesh)
    return f


def to_cells(mesh, field):
    f = field
    if field.association() == infinity.NodeAssociation():
        f = f.averageToCells(mesh)
    return f


def create_uniform_field(length, association=infinity.NodeAssociation(), name="uniform"):
    field = length*[1.0]
    return infinity.Field(name, association, field)


if __name__ == "__main__":
    infinity.initializeMPI()
    args = get_args()

    comm = infinity.getCommunicator()
    mesh = load_mesh(comm, args)

    root_print(comm, "Reading snap database: " + args.snap)

    output_name = add_extension_if_missing(args.output_name, "snap")

    input_field_names = []
    output_fields = []

    if args.snap != "":
        snap = infinity.Snap(args.snap, mesh, comm)
        input_field_names = snap.listFields()
        output_field_names = []

        if args.extract_snap != "":
            output_field_names.append(args.extract_snap)
        else:
            output_field_names = input_field_names

        for f in output_field_names:
            output_fields.append(snap.field(f))

    if args.list:
        root_print(comm, "Contains fields: ")
        for f in input_field_names:
            root_print(comm, f)
        exit(comm)

    if args.create_uniform:
        output_fields.append(create_uniform_field(mesh.nodeCount()))

    node_association_names = ["nodes", "node", "NODE", "NODES"]
    if args.at_association in node_association_names:
        for i in range(len(output_fields)):
            output_fields[i] = to_nodes(mesh, output_fields[i])

    cell_association_names = ["cells", "cell", "CELLS", "CELL"]
    if args.at_association in cell_association_names:
        for i in range(len(output_fields)):
            output_fields[i] = to_cells(mesh, output_fields[i])

    out_snap = infinity.Snap(mesh, comm)
    for f in output_fields:
       out_snap.add(f)
    out_snap.write(output_name)

    exit(comm)

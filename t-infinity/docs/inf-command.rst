.. _inf-command:

inf command
===================

The :code:`inf` command line application has many features built on T-infinity technologies.

validate
--------
The :code:`inf validate` command will perform a series of mesh checks on a given mesh.
By default the T-infinity cell-centered pre-processor plugin is used to pre-process the mesh.
A separate pre-processor plugin can be selected with :code:`--pre-processor <pre-processor plugin name>`

Some of the checks preformed include:

- All cells have positive volume
- All cell faces create a closed surface
- The mesh contains no hanging edges
- The mesh contains no hanging nodes
- All volume cells have face neighbors (no missing surface faces)
- Owned nodes are arranged first (check only performed in a fun3d.nml is found in the working directory)
- All owned nodes have full stencil support (in parallel)
- All owned cells have full stencil support (in parallel)
- All nodes are contained within at least one cell

plot
----
The :code:`inf plot` command can be used to help visualize grids and simulation results. 
One common usage is to simply look convert a ugrid file to a visualization file 
that can be read in by ParaView or TecPlot. 
::
  inf plot --mesh mygrid.lb8.ugrid -o mygrid.plt

Input mesh files
^^^^^^^^^^^^^^^^
The ascii ugrid (extension .ugrid), binary big endian ugrid (extension .b8.ugrid), and little endian
(extension .lb8.ugrid) are supported by all :code:`--pre-processor` options.

:code:`.meshb` files can be read by selecting the :code:`--pre-processor RefinePlugins` option.

Output visualization files
^^^^^^^^^^^^^^^^^^^^^^^^^^
Output file types are selected by the output extension.
The default output filename is :code:`out.vtk`.  
Output types include:
::

  .vtk    binary vtk file
  .plt    binary tecplot file
  .stl    binary stl file (only for surface plots) 
  .csv    ascii csv file (will output node data to each column)
  .dat    ascii tecplot (will output node data to each column)
  .snap   binary snap file
  .ugrid  will write a .lb8.ugrid and a .snap file for any field data.

snap file
^^^^^^^^^
The :code:`inf plot` command can read a :code:`.snap` file containing node or cell data 
and include those fields in the output files.
::
  inf plot --mesh mygrid.lb8.ugrid --snap mysimulationdata.snap
By default all fields in the snap file will be included in any output files. 
But you can also select just some fields to be included with the :code:`--select` option:
::
  inf plot --mesh mygrid.lb8.ugrid --snap mysimulationdata.snap --select Pressure

Converting between Node and Cell fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


Filters
^^^^^^^
Big simulations produce large output files. 
It can be very helpful to downselect the full computational domain before pulling 
visualization data off remote machines for viewing. 
The :code:`plot` utility offers a collection of subvolume filtering tools.  

**Note** The :code:`plot` filters are all crinkle-cut meaning if any part of a cell is included 
within the filter bounds the entire cell is included.  If you want to create actual 2D surfaces
look at the :code:`sampling` command.

Slice
"""""
Create a crinkle cut of a 2D slice.  Can support x, y, or z constant planes.
For example, an yz slice at x = 1.0:
::
  --slice x = 1.0

Sphere
""""""
Create a crinkle cut for all cells inside a sphere of radius :code:`r` centered at :code:`x y z`
::
  --sphere x y z r  


Surface
"""""""
Plot only the surface (no volume elements will be added).
::
  --surface
If :code:`--surface` is selected the boundary tags will automatically be added to the visualization 
output unless :code:`clean` is also added.  This can be helpful when you quickly want to visualize
the grid for setting boundary conditions and are unsure how boundaries are labeled.

Volume
""""""
Plot only the volume (no surface elements will be added).
::
  --volume

Tags
""""
You can plot only certain surface regions by their tag:
::
  --tags 1 2 4:10
You can define a list of tags either space or comma separated, and select rages with :code:`:`. 

Cell list
""""""""""
Include certain cells explicitly by their global ID (useful for debugging).
::
    --cell-list 1 2 4:10

Field Conversions
^^^^^^^^^^^^^^^^^
You can convert cell fields to node fields (and vice versa) using the :code:`--at` command:
::
  inf plot --at nodes
All output fields will average any cell fields to nodes using volume weighted averaging.
::
  inf plot --at cells
All output fields will average any node fields to cells using simple averaging.

Debugging Tools
^^^^^^^^^^^^^^^
The plot command has a few features that can help in debugging.

Explode
"""""""
An exploded visualization of a mesh can be made using
::
  --explode
With an exploded view the gaps between cells can be increased by using a smaller scale:
::
  --scale 0.75

Partition
"""""""""
The :code:`inf plot` command can be run in mpi parallel and any number of T-infinity pre-processors
can be selected using the :code:`--pre-processor` command.  
If you want a visualization of the partitioning by running the command in parallel and using the option:
::
  --partition 

Global Ids
""""""""""
Global Ids for cells and nodes can be added to the output fields using
::
  --gids

Smoothness
""""""""""
Plots surface smoothness as measured by the most dissimilar surface normals for all surface cells surrounding a node.
::
   --smoothness


cartmesh
--------
Creates a simple cartesian aligned hex mesh.  Useful for debugging. 

Example:

:code:`inf cartmesh --lo 0 0 0 --hi 1 1 1 --dimensions 2 4 9 -o out.lb8.ugrid`

Writes a mesh to the file "out.lb8.ugrid" that is a unit cube with 2 cells in x, 4 in y, and 9 in z.


distance
--------
Computes the distance to the surface for every node or cell in the mesh. 

Example:
:code:`inf distance -m aircraft.lb8.ugrid --at nodes --tags 1:10 -o distance.csv`

Writes a csv file containing the nearest distance to every node in the mesh from 
for surfaces 1 through 10. 

fix
---
Attempts to fix common issues in unstructured grids. 

Generate triangle and quad elements for any volume cell face where no neighboring element could be found. 
::
  --missing-faces

Reassign a group of boundary tags to merge into a single tag. 
::
  --quilt-tags 1 5:10

Reassign all boundary tags sharing the same name to a single tag.  Requires a .mapbc file
::
  --lump-bcs mapbc-filename

Attempt to remove hanging edges present grids generated by some grid generation tools.
::
  --hang-edge

Reorder cells in the mesh based on the Reverse Cuthill-McKee reordering.
::
  --reorder
        
opt (Under Development)
-------------------
A very simple tool that attempts to improve grid quality by moving node locations.

Allow certain nodes on the surface to slide along the surface. 
Nodes on the boundary between surface tags will not move (see --quilt-tags)
::
  --slip-tags 1 5 7

Treat a set of tags as if they were a single tag alowing nodes to slide between an of the specified tags. 
::
  --quilt-tags 1 5 7

Select the smoothing algorithm to use: hilbert (based on Bruce Hilbert's PhD thesis) or smooth (simple algebraic node smoothing).
::
  --algorithm hilbert or smooth

Select the default node step size as a percent fraction of the local mesh spacing
::
  --beta 0.01

Plot after each iteration (slow, but helpful in visualizing changes)
::
  --plot

Smooth regardless of changes in the Hilbert cost function.  Helpful in quickly trying to untangle really tangled meshes. 
::
  --smooth

Disable trying to explicitly flatten quad faces (may help reduce creating tangled meshes)
::
  --disable-warp

oml-translate 
-------------
A developer tool used to convert .voml files to .json files. 

examine
-------
Examines a mesh file and outputs simple metrics such as mesh size and optionally the boundary tags. 

transform
---------
Move a mesh by scaling, translating or rotating about Cartesian axis. 


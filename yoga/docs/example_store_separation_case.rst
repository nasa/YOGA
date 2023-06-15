Store Separation Example
========================

This tutorial demonstrates how to run the `FUN3D store separation overset tutorial <https://fun3d.larc.nasa.gov/tutorial-2.html#overset_moving_grids>`_ using Yoga as the domain assembler.

`Download tutorial case <https://fun3d.larc.nasa.gov/Wing_Store.tar.gz>`_
and upload to K.

Load module with Yoga executable

.. code-block:: bash

    module load --auto t-infinity/latest

Run the yoga executable to combine the wing and store grids
into a composite grid with the basename 'wingstore':

.. code-block:: bash

    cd Grids
    yoga make-composite --file composite_grid_recipe.txt -o wingstore

The overset namelist for this case in `fun3d.nml_steady`
has the default values overwritten with the appropriate
values for running with Yoga (by default, FUN3D will use Suggar++).

.. code-block:: fortran

    &overset_data
        assembler = 'yoga'
        dci_on_the_fly = .true.
        overset_flag = .true.
        input_imesh_file = 'imesh.dat'
    /


Create symbolic links for the grids and imesh file in `Steady/`

.. code-block:: bash

    cd Steady
    ln -s ../Grids/wingstore.mapbc .
    ln -s ../Grids/wingstore.lb8.ugrid .
    ln -s ../Grids/imesh.dat .



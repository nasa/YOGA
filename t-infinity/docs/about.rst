T-infinity
==========
T-infinity is an framework for simulation applications.

.. image:: _static/images/t-inf-blue-ring.png
   :width: 500px
   :align: center
   :alt: T-infinity overview


What is T-infinity?
-------------------
T-infinity increases collaboration.

No really, what is T-infinity?
------------------------------
T-infinity is a core set of C++ libraries that standardize how simulation tools interoperate. 

Um.... what is T-infinity?
--------------------------
T-infinity is a motherboard for CFD.

The motherboard metaphor
^^^^^^^^^^^^^^^^^^^^^^^^
T-infinity is like a motherboard - but for CFD software. 
T-infinity lets different components of CFD software talk to each other just like a motherboard lets the CPU talk to a hard drive. 
If your computer uses a standard motherboard you can go buy any off the shelf hard drive and use it!

If you don't use standards you would have to custom build a hard drive.  Are **you** an expert on hard drive design and manufacturing?  
No?  Do you want to be?! Hmmm, still no?  You better use standards. 

T-infinity is not a CFD solver.
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

T-infinity is somewhat modeled after the Department of Energy's Trilinos project. 
Different software packages can be used as building blocks to develop simulation applications. 

Supprorting our legacy - and the future
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
T-infinity components are designed to be modular.  
Modular means today's CFD solvers can pickup just one feature from T-infinty and incorporate it. 
For example, for RANS based CFD every solver needs an accurate wall distance calculation. 
Quickly, and accurately calculating wall distance on very large CFD grids is a challenging computer science problem.
T-infinity has a distance calculator component. It has been incorporated into:

- VULCAN
- Laura
- HyperSolve
- SFE
- FUN3D
Now all these codes leverage don't have to write their own.  
An improvement in the shared T-infinity calculator has immediate impact across all these solvers. 

Legacy CFD solvers do not have to be rewritten from scratch to leverage some of the capabilities of T-infinity. 
The tools are modular and cohesive.  The more you buy-in the more you benefit.  




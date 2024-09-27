# YOGA - YOGA is an Overset Grid Assembler

The YOGA Overset Grid Assembler is no longer developed.  Source code is provided "as-is" and no support is available.

YOGA has a required dependency on Nanoflan version 1.3.0.

To configure and build with GNU Autotools, you will have to first bootstrap the raw source after cloning the repository.  This is typically only required after the initial clone.

```
% git clone git@github.com:nasa/yoga.git
% cd yoga
% ./bootstrap
% mkdir _build
% cd _build
% ../configure --prefix=`pwd` \
               --with-mpi=/path/to/mpi \
               --with-nanoflann=/path/to/nanoflann/1.3.0
% make -j
% make install
```

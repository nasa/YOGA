Parfait is an unstructured mesh toolkit developed at NASA Langley.


# Building
The Parfait toolkit is header only. If you just want to use the C++ toolkit simply

```
mkdir build
cd build
cmake .. -DHEADER_ONLY=TRUE
make -j install
```

## Dependencies
Parfait depends on C++14.  We suggest you use the latest GNU compiler.  We regularly test with GCC 6.2 and 7.0.
The CMake build system used in Parfait is under development.  Defining dependencies is not uniform.  Sorry for the mess.

### MPI
All Parfait utilities currectly rely on MPI.  Please make sure MPI is added to your `LD_LIBRARY_PATH` and `INCLUDE_PATH` (on many linux systems you simply `module load ...` your favorite flavor of MPI).

### Catch2
To enable building Parfait's unit tests you must have Catch2 installed. 
You can install Catch2 using their provided cmake and add the FindCatch2.cmake path to your `CMAKE_PREFIX_PATH` environment variable.
Or, you can simple install the Catch2 catch.hpp header file, and build with:

```
mkdir build
cd build
cmake .. -DCatch2_path=/path/to/catch.hpp
make -j install
```


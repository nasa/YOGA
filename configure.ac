# -*- Autoconf -*- to force emacs autoconf mode

# Process this file with autoconf to produce a configure script.

define([git_describe], esyscmd([sh -c "git rev-parse --short HEAD | tr -d '\n'"]))dnl

AC_INIT([YOGA],
        [1.0-git_describe],
        [fun3d-support@lists.nasa.gov],
        [yoga])

AC_CONFIG_HEADERS(config.h)
AC_PREREQ(2.63)
AC_COPYRIGHT([This material is declared a work of the U.S. Government
and is not subject to copyright protection in the United States. However,
the data within is Export Controlled.])

AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR([src/yoga/yoga_c_interface.cpp])

AX_CONFIG_PKG_DIR

AX_BUILD_TOOLS_LOCATION

AC_CONFIG_MACRO_DIR(config)
AC_SUBST([ACLOCAL_AMFLAGS], ["-I ./config -I ${ax_cv_path_build_tools}/aclocal \${ACLOCAL_FLAGS}"])

AC_CANONICAL_TARGET

AH_TOP([/* Required Definitions for YOGA */])

# Check for MPI (may change FC, CC, and CXX)
MPI_LIB_PATH

AM_INIT_AUTOMAKE([1.11 tar-pax -Wno-portability])

AC_PROG_CXX(icpc g++)

AX_CXX_COMPILE_STDCXX([14],[noext],[mandatory])

# Use libtool to create shared object
LT_PREREQ([2.4.2])
LT_EXTENSION()
LT_INIT([disable-shared])

# Check for pancake
AS_VAR_SET_IF([with_pancake],[],[AS_VAR_SET([with_pancake],["subpackage"])])
AX_PANCAKE_REQUIRED

AX_PARFAIT
AX_MESSAGE_PASSER
AX_TRACER
AX_TINFINITY

AX_CPPZMQ

# Done with checks

AC_CONFIG_FILES([Makefile \
	src/Makefile \
	src/nanoflann/Makefile \
	src/ddata/Makefile \
	src/yoga/Makefile \
	])

AC_OUTPUT

# Summary

MPISUPPORT=`if test "$with_mpi" != 'no'; then echo yes ; else echo no ; fi`

AC_MSG_RESULT([

Configuration (YOGA Library):

      Source code location: ${srcdir}
      C++ Compiler:         ${CXX}
      C++ flags:            ${CXXFLAGS}
      Linker flags:         ${LDFLAGS} ${F90_EXT_LIB}
      Dependencies:         ${dep_type}

bindings:

      MPI support:           ${MPISUPPORT}
      Parfait support:       ${with_parfait}
      MessagePasser support: ${with_message_passer}
      Tracer support:        ${with_tracer}
      PANCAKE support:       ${with_pancake}
])

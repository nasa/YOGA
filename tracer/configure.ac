# -*- Autoconf -*-

AC_PREREQ(2.53)

AC_INIT([tracer], [1.0], [fun3d-support@lists.nasa.gov])

AC_CONFIG_SRCDIR([include/Tracer.h])
AM_INIT_AUTOMAKE([foreign tar-pax])

AC_PROG_CC(icc gcc)
AC_PROG_CXX(icpc armclang++ g++)
AX_TUNE_CXX
AX_CXX_COMPILE_STDCXX([14], [noext], [mandatory])
AC_HEADER_STDC
AM_PROG_CC_C_O

LT_PREREQ([2.4.2])
LT_INIT([dlopen disable-shared])


AC_CONFIG_FILES( \
	   Makefile \
	   include/Makefile
	   )

AC_OUTPUT

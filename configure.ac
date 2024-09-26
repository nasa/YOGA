AC_INIT([YOGA],
        [1.0],
        [fun3d-support@lists.nasa.gov],
        [yoga])

AC_PREREQ(2.69)

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR([yoga/src/yoga_c_interface.h])

AC_CONFIG_MACRO_DIR([config])

AM_INIT_AUTOMAKE([foreign tar-pax])

AX_CXX_COMPILE_STDCXX([14], [noext], [mandatory])
AM_PROG_CC_C_O
AC_PROG_CXX(icpc armclang++ g++)
AX_TUNE_CXX


dnl mpi autostuff
AC_ARG_WITH(mpi,
        [[  --with-mpi[=ARG]        Path to MPI library installation [ARG=no]]],
        [with_mpi="$withval"],      [with_mpi="no"] )
AC_ARG_WITH(mpicc,
        [[  --with-mpicc[=ARG]      MPI C compiler wrapper absolute path [mpicc]]],
        [with_mpicc="$withval"],    [with_mpicc="mpicc"] )
AC_ARG_WITH(mpicxx,
        [[  --with-mpicxx[=ARG]     MPI C++ compiler wrapper absolute path [mpicxx]]],
        [with_mpicxx="$withval"],   [with_mpicxx="mpicxx"] )
AC_ARG_WITH(mpibin,
        [[  --with-mpibin[=ARG]     MPI bin directory [bin]]],
        [with_mpibin="$withval"],   [with_mpibin="bin"] )
AC_ARG_WITH(mpiinc,
        [[  --with-mpiinc[=ARG]     MPI include directory for mpi.h [include]]],
        [with_mpiinc="$withval"],   [with_mpiinc="include"] )
AC_DEFINE_UNQUOTED([MPI_PATH],["$with_mpi"],[MPI installation])
if test "$with_mpi" != 'no'
then
  AC_MSG_NOTICE([checking MPI installation])
  if test "$with_mpiinc" != 'no'
  then
    if test $(echo $with_mpiinc | grep /)
    then
      mpi_include="-I$with_mpiinc"
    else
      mpi_include="-I$with_mpi/$with_mpiinc"
    fi
  fi
  AC_SUBST([mpi_include])

  if test "$with_mpibin" != 'no'
  then
    if test $(echo $with_mpibin | grep /)
    then
      MPIBIN="$with_mpibin"
    else
      MPIBIN="$with_mpi/$with_mpibin"
    fi
  fi

  if test "$with_mpicc" != 'no'
  then
    AC_MSG_NOTICE([checking mpi C compiler wrapper...])
    if test $(echo $with_mpicc | grep /)
    then
      MPICC="$with_mpicc"
    else
      MPICC="$MPIBIN/$with_mpicc"
    fi
    AC_MSG_RESULT([  $MPICC])
    if eval "test -e $MPICC"
    then
      CC=$MPICC
    else
      AC_MSG_ERROR([MPI C compiler wrapper does not exist])
    fi

    mpi_ldadd=''
    tokens=($($MPICC -show))
    for i in $[{tokens[*]}]
    do
      mpi_ldadd="$mpi_ldadd $(AS_ECHO([$i]) | grep -E -- '-l|-L' 2> /dev/null)"
    done

  else
    # when wrapper is missing (i.e. sgi mpt) link with -lmpi
    mpi_ldadd="-L${with_mpi}/lib -lmpi"
  fi
  AC_SUBST([MPICC])
  AC_SUBST([mpi_ldadd])

  if test "$with_mpicxx" != 'no'
  then
    AC_MSG_NOTICE([checking mpi C++ compiler wrapper...])
    if test $(echo $with_mpicxx | grep /)
    then
      MPICXX="$with_mpicxx"
    else
      MPICXX="$MPIBIN/$with_mpicxx"
    fi
    AC_MSG_RESULT([  $MPICXX])
    if eval "test -e $MPICXX"
    then
      CXX=$MPICXX
    else
      AC_MSG_ERROR([MPI C++ compiler wrapper does not exist])
    fi

    mpixx_ldadd=''
    tokens=($($MPICXX -show))
    for i in $[{tokens[*]}]
    do
      mpixx_ldadd="$mpixx_ldadd $(AS_ECHO([$i]) | grep -E -- '-l|-L' 2> /dev/null)"
    done

  else
    # when wrapper is missing (i.e. sgi mpt) link with -lmpi
    mpixx_ldadd="-L${with_mpi}/lib -lmpi"
    AC_CACHE_CHECK([MPI C++ Bindings],
    [ax_cv_mpi_cxx_bindings],
    [ax_check_mpi_cxx_bindings_LIBS=$LIBS
     LIBS="-lmpi++ $mpixx_ldadd $LIBS"
     AC_LANG_PUSH([C++])
     AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <mpi++.h>]],
                                     [[  PMPI::Init();]])],
                    [ax_cv_mpi_cxx_bindings=yes],
                    [ax_cv_mpi_cxx_bindings=no])
     AC_LANG_POP([C++])
     LIBS=$ax_check_mpi_cxx_bindings_LIBS
    ])

    if test "$ax_cv_mpi_cxx_bindings" != 'no'
    then
      mpixx_ldadd="-lmpi++ $mpixx_ldadd"
    fi
    AC_SUBST([mpixx_ldadd])
  fi
  AC_SUBST([MPICXX])
  AC_SUBST([mpixx_ldadd])

  AM_CONDITIONAL(BUILD_MPI,true)
else
  AC_MSG_WARN([MPI library installation not found, use --with-mpi])
  AM_CONDITIONAL(BUILD_MPI,false)
fi

LT_PREREQ([2.4.2])
LT_INIT([dlopen shared])

AC_ARG_WITH(nanoflann,
	[  --with-nanoflann[=ARG]          use nanoflann [ARG=no]],
	[with_nanoflann=${withval}],      [with_nanoflann="no"])

if test "${with_nanoflann}" != 'no'
then
    AC_CHECK_FILE([${with_nanoflann}/include/nanoflann.hpp],
                  [nanoflann_h_path=${with_nanoflann}/include],
                  [nanoflann_h_path='no'])
 if test "${nanoflann_h_path}" != 'no'
  then
    nanoflann_include="-I${nanoflann_h_path}"
    AC_SUBST([nanoflann_include])
    AM_CONDITIONAL(BUILD_NANOFLANN,true)
  else
    AC_MSG_WARN([nanoflann not found, use --with-nanoflann])
    AM_CONDITIONAL(BUILD_NANOFLANN,false)
  fi
else
  AC_MSG_WARN([nanoflann not found, use --with-nanoflann])
  AM_CONDITIONAL(BUILD_NANOFLANN,false)
fi

AX_PARMETIS

# AC_ARG_WITH(parmetis,
# 	[  --with-parmetis[=ARG]     use ParMETIS partitioner [ARG=no]],
# 	[with_parmetis=$withval],        [with_parmetis="no"])

# if test "$with_parmetis" != 'no'
# then
#   AM_CONDITIONAL(BUILD_PARMETIS_SUPPORT,true)
# else
#   AM_CONDITIONAL(BUILD_PARMETIS_SUPPORT,false)
# fi

MessagePasser_include="-I\$(top_srcdir)/MessagePasser"
AC_SUBST([MessagePasser_include])
parfait_include="-I\$(top_srcdir)/parfait"
AC_SUBST([parfait_include])
tracer_include="-I\$(top_srcdir)/tracer/include"
AC_SUBST([tracer_include])
t_infinity_include="-I\$(top_srcdir)/t-infinity/src"
AC_SUBST([t_infinity_include])
ddata_include="-I\$(top_srcdir)/ddata"
AC_SUBST([ddata_include])
t_infinity_interfaces_include="-I\$(top_srcdir)/t-infinity/src/t-infinity-interfaces"
AC_SUBST([t_infinity_interfaces_include])
plugin_parfait_include="-I\$(top_srcdir)/plugin-parfait"
AC_SUBST([plugin_parfait_include])

# plugin_parfait_path="\$(top_builddir)/plugin-parfait"
parfait_path="\$(abs_top_builddir)/parfait"
AC_SUBST([parfait_path])
plugin_parfait_path="\$(abs_top_builddir)/plugin-parfait"
AC_SUBST([plugin_parfait_path])
t_infinity_path="\$(top_builddir)/t-infinity"
AC_SUBST([t_infinity_path])
tracer_path="\$(top_builddir)/tracer"
AC_SUBST([tracer_path])

AM_CONDITIONAL([BUILD_PLUGIN], [test x"${enable_shared}" != x"no"])
AM_CONDITIONAL([BUILD_YOGA_APP], [true])

AC_CONFIG_FILES( \
tracer/include/Makefile \
tracer/Makefile \
yoga/Makefile \
yoga/src/Makefile \
yoga/utilities/Makefile \
yoga/doc/Makefile \
yoga/doc/manual/Makefile \
ddata/ddata/Makefile \
ddata/Makefile \
MessagePasser/Makefile \
Makefile \
parfait/Makefile \
parfait/parfait/Makefile \
t-infinity/Makefile \
t-infinity/src/Makefile \
t-infinity/src/utilities/Makefile \
t-infinity/src/utilities/DistanceCalculator/Makefile \
t-infinity/src/t-infinity/Makefile \
t-infinity/src/t-infinity-interfaces/Makefile \
t-infinity/src/t-infinity-interfaces/c-pre-processor/Makefile \
t-infinity/src/t-infinity-interfaces/c-grouping-pre-processor/Makefile \
plugin-parfait/Makefile \
plugin-parfait/shared/Makefile \
plugin-parfait/Viz/Makefile \
plugin-parfait/SerialPreProcessor/Makefile \
plugin-parfait/NodeCenteredPreProcessor/Makefile \
)

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

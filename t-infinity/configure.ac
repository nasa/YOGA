# -*- Autoconf -*-

AC_PREREQ(2.53)

AC_INIT([t-infinity], [1.0], [fun3d-support@lists.nasa.gov])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_SRCDIR([src/t-infinity/MeshInterface.h])
AM_INIT_AUTOMAKE([foreign tar-pax])

AC_PROG_CC(icc gcc)
AX_TUNE_CXX
AC_PROG_CXX(icpc armclang++ g++)
AX_CXX_COMPILE_STDCXX([14], [noext], [mandatory])
AC_HEADER_STDC
AM_PROG_CC_C_O

LT_PREREQ([2.4.2])
LT_INIT([dlopen disable-shared])


dnl mpi autostuff
AC_ARG_WITH(mpi,
        [[  --with-mpi[=ARG]        Path to MPI library installation [ARG=no]]],
        [with_mpi="$withval"],      [with_mpi="no"] )
AC_ARG_WITH(mpicc,
        [[  --with-mpicc[=ARG]      MPI C compiler wrapper absolute path [with_mpi/bin/mpicc]]],
        [with_mpicc="$withval"],    [with_mpicc="with_mpi/bin/mpicc"] )
AC_ARG_WITH(mpiinc,
        [[  --with-mpiinc[=ARG]     MPI include absolute path for mpif.h [with_mpi/include]]],
        [with_mpiinc="$withval"],   [with_mpiinc="with_mpi/include"] )
if test "$with_mpi" != 'no'
then
  AC_MSG_NOTICE([checking MPI installation])
  if test "$with_mpiinc" != 'no'
  then
    if test "$with_mpiinc" = 'with_mpi/include'
    then
      mpi_include="-I${with_mpi}/include"
    else
      mpi_include="-I${with_mpiinc}"
    fi
  else
    mpi_include=""
  fi
  AC_SUBST([mpi_include])

  if test "$with_mpicc" != 'no'
  then
    if test "$with_mpicc" = 'with_mpi/bin/mpicc'
    then
      MPICC="${with_mpi}/bin/mpicc"
    else
      MPICC="${with_mpicc}"
    fi
    mpi_ldadd="-L${with_mpi}/lib -lmpi"
  else
    MPICC="${CC}"
    mpi_ldadd="-L${with_mpi}/lib -lmpi"
  fi
  AC_SUBST([MPICC])
  AC_SUBST([mpi_ldadd])

  AM_CONDITIONAL(BUILD_MPI,true)
else
  AC_MSG_ERROR([Required MPI library installation not found, use --with-mpi])
  AM_CONDITIONAL(BUILD_MPI,false)
fi

AC_ARG_WITH(MessagePasser,
	[  --with-MessagePasser[=ARG]    use MessagePasser.h [ARG=no]],
	[with_MessagePasser=${withval}],      [with_MessagePasser="no"])

if test "${with_MessagePasser}" != 'no'
then
    AC_CHECK_FILE([${with_MessagePasser}/MessagePasser/Message.h],
                  [MessagePasser_h_path=${with_MessagePasser}],
                  [MessagePasser_h_path='no'])
 if test "${MessagePasser_h_path}" != 'no'
  then
    MessagePasser_include="-I${MessagePasser_h_path}"
    AC_SUBST([MessagePasser_include])
  else
    AC_MSG_ERROR([Required MessagePasser not found, use --with-MessagePasser])
  fi
else
    AC_MSG_ERROR([Required MessagePasser not found, use --with-MessagePasser])
fi

AC_ARG_WITH(parfait,
	[  --with-parfait[=ARG]          use parfait [ARG=no]],
	[with_parfait=${withval}],      [with_parfait="no"])

if test "${with_parfait}" != 'no'
then
  parfait_path="${with_parfait}"
  AC_SUBST([parfait_path])
  AC_CHECK_FILE([${with_parfait}/parfait/Point.h],
                [parfait_h_path=${with_parfait}],
                [parfait_h_path='no'])
  if test "${parfait_h_path}" != 'no'
  then
    parfait_include="-I${parfait_h_path}"
    AC_SUBST([parfait_include])
  else
    AC_MSG_ERROR([Required parfait not found, use --with-parfait])
  fi

  AC_CHECK_FILE([${with_parfait}/lib/libparfait.a],[have_parfait_lib="yes"],
                [have_parfait_lib="no"])
  AM_CONDITIONAL([BUILD_YOGA_APP], [test "x$have_parfait_lib" != "xno"])
else
    AC_MSG_ERROR([Required parfait not found, use --with-parfait])
fi

AC_ARG_WITH(tracer,
	[  --with-tracer[=ARG]          use tracer [ARG=no]],
	[with_tracer=${withval}],      [with_tracer="no"])

if test "${with_tracer}" != 'no'
then
    AC_CHECK_FILE([${with_tracer}/include/Tracer.h],
                  [tracer_h_path=${with_tracer}/include],
                  [tracer_h_path='no'])
 if test "${tracer_h_path}" != 'no'
  then
    tracer_include="-I${tracer_h_path}"
    AC_SUBST([tracer_include])
  else
    AC_MSG_ERROR([Required tracer not found, use --with-tracer])
  fi
else
    AC_MSG_ERROR([Required tracer not found, use --with-tracer])
fi

AC_ARG_WITH(parmetis,
	[  --with-parmetis[=ARG]     use ParMETIS partitioner [ARG=no]],
	[with_parmetis=$withval],        [with_parmetis="no"])

if test "$with_parmetis" != 'no'
then
  AM_CONDITIONAL(BUILD_PARMETIS_SUPPORT,true)
else
  AM_CONDITIONAL(BUILD_PARMETIS_SUPPORT,false)
fi

t_infinity_include="-I\$(top_srcdir)/src"
AC_SUBST([t_infinity_include])
t_infinity_interfaces_include="-I\$(top_srcdir)/src/t-infinity-interfaces"
AC_SUBST([t_infinity_interfaces_include])

AM_CONDITIONAL([BUILD_PLUGIN], [test x"${enable_shared}" != x"no"])

AC_CONFIG_FILES( \
		 Makefile \
		 src/Makefile \
       	         src/t-infinity/Makefile \
       	         src/utilities/Makefile \
                 src/utilities/DistanceCalculator/Makefile \
                 src/t-infinity-interfaces/Makefile \
                 src/t-infinity-interfaces/c-pre-processor/Makefile
	   	 )


AC_OUTPUT

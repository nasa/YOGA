# -*- Autoconf -*-

AC_DEFUN([_AX_GKLIB_],[

AC_ARG_WITH(gklib,
	[  --with-gklib[=ARG]        use GKlib support lib],
	[with_gklib=$withval],       [with_gklib="no"])

if test "$with_gklib" != 'no'
then

  AC_CHECK_FILE([$with_gklib/include/GKlib.h],
                [gklib_h_path=$with_gklib/include],
                [gklib_h_path='no'])

  AC_CHECK_FILE([$with_gklib/lib/libGKlib.a],
                [gklib_a_path=$with_gklib/lib],
		[gklib_a_path='no'])

  if test "$gklib_h_path" != 'no'
  then
    gklib_include="-I$gklib_h_path"
  else
    AC_MSG_WARN([GKlib.h not found in $with_gklib/include])
  fi

  gklib_ldadd=""
  if test "$gklib_a_path" != 'no'
  then
      gklib_ldadd="-L${gklib_a_path} -lGKlib"
  fi
  if test "empty$gklib_ldadd" == 'empty'
  then
    AC_MSG_WARN([libgklib.a not found in $with_gklib/lib])
    AM_CONDITIONAL(BUILD_GKLIB_SUPPORT,false)
  else
    AC_DEFINE([HAVE_GKLIB],[1],[GKLIB is available])
    AM_CONDITIONAL(BUILD_GKLIB_SUPPORT,true)
  fi

  AC_SUBST([gklib_include])
  AC_SUBST([gklib_ldadd])
else
  AM_CONDITIONAL(BUILD_GKLIB_SUPPORT,false)
fi

])

AC_DEFUN([_AX_METIS_],[

AC_ARG_WITH(metis,
	[  --with-metis[=ARG]        use METIS partitioner],
	[with_metis=$withval],       [with_metis="no"])

if test "$with_metis" != 'no'
then

  AC_CHECK_FILE([$with_metis/include/metis.h],
                [metis_h_path=$with_metis/include],
                [metis_h_path='no'])

  AC_CHECK_FILE([$with_metis/lib/libmetis.a],
                [metis_a_path=$with_metis/lib],
		[metis_a_path='no'])
  AC_CHECK_FILE([$with_metis/lib/libmetis.so],
                [metis_so_path=$with_metis/lib],
		[metis_so_path='no'])
  AC_CHECK_FILE([$with_metis/lib/libmetis.dylib],
                [metis_dylib_path=$with_metis/lib],
		[metis_dylib_path='no'])

  if test "$metis_h_path" != 'no'
  then
    metis_include="-I$metis_h_path"
  else
    AC_MSG_ERROR([metis.h not found in $with_metis/include])
  fi

  metis_ldadd=""
  if test "$metis_a_path" != 'no'
  then
      metis_ldadd="-L${metis_a_path} -lmetis"
  fi
  if test "$metis_so_path" != 'no'
  then
      metis_ldadd="-L${metis_so_path} -lmetis"
  fi
  if test "$metis_dylib_path" != 'no'
  then
      metis_ldadd="-L${metis_dylib_path} -lmetis"
  fi
  if test "empty$metis_ldadd" == 'empty'
  then
    AC_MSG_ERROR([libmetis.{a,so,dylib} not found in $with_metis/lib])
  fi

  AC_SUBST([metis_include])
  AC_SUBST([metis_ldadd])
  AC_DEFINE([HAVE_METIS],[1],[METIS is available])
  AM_CONDITIONAL(BUILD_METIS_SUPPORT,true)
else
  AM_CONDITIONAL(BUILD_METIS_SUPPORT,false)
fi

])

AC_DEFUN([AX_PARMETIS],[

AC_ARG_WITH(parmetis,
	[  --with-parmetis[=ARG]     use ParMETIS partitioner [ARG=no]],
	[with_parmetis=$withval],    [with_parmetis="no"])

AS_VAR_SET_IF([with_gklib],
              [AS_VAR_IF([with_gklib],[no],
                         [AS_VAR_SET([with_gklib],[$with_parmetis])])],
              [AS_VAR_SET([with_gklib],[$with_parmetis])])
AS_VAR_SET_IF([with_metis],
              [AS_VAR_IF([with_metis],[no],
                         [AS_VAR_SET([with_metis],[$with_parmetis])])],
              [AS_VAR_SET([with_metis],[$with_parmetis])])

_AX_GKLIB_
_AX_METIS_

if test "$with_parmetis" != 'no'
then
  AC_CHECK_FILE([$with_parmetis/include/parmetis.h],
                [parmetis_h_path=$with_parmetis/include],
                [parmetis_h_path='no'])

  AC_CHECK_FILE([$with_parmetis/lib/libparmetis.a],
                [parmetis_a_path=$with_parmetis/lib],
		[parmetis_a_path='no'])
  AC_CHECK_FILE([$with_parmetis/lib/libparmetis.so],
                [parmetis_so_path=$with_parmetis/lib],
		[parmetis_so_path='no'])
  AC_CHECK_FILE([$with_parmetis/lib/libparmetis.dylib],
                [parmetis_dylib_path=$with_parmetis/lib],
		[parmetis_dylib_path='no'])

  if test "$parmetis_h_path" != 'no'
  then
    parmetis_include="-I${parmetis_h_path}"
  else
    AC_MSG_ERROR([parmetis.h not found in $with_parmetis/include])
  fi

  parmetis_ldadd=""
  if test "$parmetis_a_path" != 'no'
  then
      parmetis_ldadd="-L${parmetis_a_path} -lparmetis"
  fi
  if test "$parmetis_so_path" != 'no'
  then
      parmetis_ldadd="-L${parmetis_so_path} -lparmetis"
  fi
  if test "$parmetis_dylib_path" != 'no'
  then
      parmetis_ldadd="-L${parmetis_dylib_path} -lparmetis"
  fi
  if test "empty$parmetis_ldadd" == 'empty'
  then
    AC_MSG_ERROR([libparmetis.{a,so,dylib} not found in $with_parmetis/lib])
  fi

  AC_DEFINE([HAVE_PARMETIS],[1],[ParMETIS is available])
  AM_CONDITIONAL(BUILD_PARMETIS_SUPPORT,true)
else
  AM_CONDITIONAL([BUILD_PARMETIS_SUPPORT], [test "x$with_metis" != xno])
fi

parmetis_include="${parmetis_include} ${metis_include} ${gklib_include}"
parmetis_ldadd="${parmetis_ldadd} ${metis_ldadd} ${gklib_ldadd}"

AC_SUBST([parmetis_include])
AC_SUBST([parmetis_ldadd])

])


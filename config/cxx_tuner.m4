# autoconf macros for "tuning" the cxx compilers by setting appropriate
# options and #defines
#
# NOTE: This macro will augment $CXXFLAGS and potentially $LDFLAGS!
#
# Assigned Output Variables:
#   @CXX_EXT_LIB@		CXX extensions library
#   @openmp_flags@		OpenMP compiler flags
#

AC_DEFUN([AX_TUNE_CXX],[

AC_ARG_ENABLE(cxxtune,
        [[  --enable-cxxtune        tailor CXX compiler options for FUN3D [yes]]],
        [enable_cxxtune=$enableval],  [enable_cxxtune="yes"])

CXX_EXT_LIB=$CXX_EXT_LIB

compiler_base=$(basename $CXX)
AC_MSG_RESULT([  Tuning $compiler_base compiler for FUN3D])

if test -n "${MPICXX}" -a -n "$(echo ${CXX}x | grep ${MPICXX}x)"
then
  compiler_base=$($CXX -show | grep -v "^ln" | grep -v "^rm")
  compiler_base=$(echo "$compiler_base" | sed 's/ .*//')
  compiler_base=$(basename "$compiler_base")
  compiler_base=$(echo $compiler_base | sed 's/ .*//')
else
  compiler_base=$(echo $compiler_base | sed 's/ .*//')
fi
AC_DEFINE_UNQUOTED([CXX_COMPILER],["$compiler_base"],[Base CXX Compiler])

openmp_flags=""


AC_MSG_RESULT([  enable_cxxtune = ${enable_cxxtune}])
if test "$enable_cxxtune" != 'no'
then
  AX_DEFAULT_CXXFLAGS
fi

case $compiler_base in
  icpc)

    if test "$enable_cxxtune" != 'no'
    then
      CXXFLAGS="-O3 $CXXFLAGS"
    fi
    openmp_flags="-qopenmp"

    CXX_EXT_LIB="-lm $CXX_EXT_LIB"

    if test "$with_mpicxx" == 'no'
    then
      CXX_EXT_LIB="$CXX_EXT_LIB -lmpi"
    fi
    ;;

  g++)

    if test "$enable_cxxtune" != 'no'
    then
      CXXFLAGS="-O3 $CXXFLAGS"
    fi
    openmp_flags="-fopenmp"

    CXX_EXT_LIB="-lm $CXX_EXT_LIB"

    if test "$with_mpicxx" == 'no'
    then
      CXX_EXT_LIB="$CXX_EXT_LIB -lmpi"
    fi
    ;;

  pgc*)

    if test "$enable_cxxtune" != 'no'
    then
      CXXFLAGS="-O3 $CXXFLAGS"
    fi
    openmp_flags=""

    CXX_EXT_LIB="-lm $CXX_EXT_LIB"

    if test "$with_mpicxx" == 'no'
    then
      CXX_EXT_LIB="$CXX_EXT_LIB -lmpi"
    fi
    ;;

esac

AC_SUBST([CXX_EXT_LIB])
AC_SUBST([openmp_flags])

])

# autoconf macros for ensuring that if CXXFLAGS are unset in the shell
# where the configure command is run and are not specified in the configure
# command that CXXFLAGS will not be set to '-O2 -g' by autoconf
#
# Assigned Output Variables:
#   @CXXFLAGS@		CXX compiler options
#

AC_DEFUN([AX_DEFAULT_CXXFLAGS],[

AC_MSG_RESULT([ ac_configure_args = ${ac_configure_args}])

case $ac_configure_args in
  *CXXFLAGS*)
    AC_MSG_RESULT([  Propagating CXXFLAGS found in configure command = ${ac_configure_args}])
    ;;

  *)
    AC_MSG_RESULT([  CXXFLAGS were not found in configure command = ${ac_configure_args}])
    AS_VAR_SET([CXXFLAGS],[""])
    ;;

esac

])
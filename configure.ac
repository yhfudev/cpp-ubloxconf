#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([gpsutils], [1.0.0], [yhfudev@gmail.com])
AC_CONFIG_SRCDIR([app/ubloxconf.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE(-Wall subdir-objects parallel-tests color-tests)

# Specify the directory of additional local Autoconf macros.
AC_CONFIG_MACRO_DIR([m4])

#magic for conditional check in Makefile:
MK=''; AC_SUBST(MK)
SED=sed


# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC(clang llvm-gcc gcc)
AC_PROG_CXX(clang++ llvm-g++ g++)
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
LT_PREREQ([2.2])
LT_INIT([shared static])
#LT_INIT([disable-static])
LT_LANG([C])
LT_LANG([C++])

AC_CHECK_LIB([z], [main])


dnl Disable doc generation with doxygen option
AC_ARG_ENABLE(doxygen-doc,
              [AC_HELP_STRING([--disable-doxygen-doc],
                              [disable document generation with doxygen])],
              , enable_doxygen_doc="yes")
if test "x$enable_doxygen_doc" = "xyes"; then
  AC_CHECK_PROG(DOXYGEN,doxygen,doxygen,no)
  if test "x$DOXYGEN" = "xno"; then
    enable_doxygen_doc="no"
  fi
fi
dnl where to install the doxygen-generated HTML doc
AC_ARG_WITH(html-docdir,
            [AC_HELP_STRING([--with-html-docdir=DIR],
                            [where to install the doxyten-generated HTML doc [PREFIX/share/doc/libedio24/html]])],
            [htmldocdir="$withval"], [htmldocdir=\$\{prefix\}/share/doc/libedio24/html])
AC_SUBST(htmldocdir)
AM_CONDITIONAL(ENABLE_DOXYGEN_DOC,test "x$enable_doxygen_doc" = "xyes")



AC_MSG_CHECKING([for native Win32])
case "$host_os" in
  *cygwin* | *msys* | *mingw* | *pw32 | *cegcc*)
    native_win32=yes
    ;;
  *)
    native_win32=no
    ;;
esac
AC_MSG_RESULT([$native_win32])
AM_CONDITIONAL(OS_WIN32, test "x$native_win32" = "xyes")
if test x${native_win32} = xyes; then
  LIBS="${LIBS} -lws2_32 -lpsapi -liphlpapi -lshell32 -luserenv -luser32"
fi

# ciut
AC_ARG_WITH(ciut,
            [AS_HELP_STRING([--with-ciut],[Base directory to look for ciut headers and libraries])],
            [CIUT_LIB_DIR="${withval}/lib"
             CIUT_INC_DIR="${withval}/include"],)

AC_ARG_WITH(ciut-lib,
            [AS_HELP_STRING([--with-ciut-lib],[Directory to look for ciut libraries])],
            [CIUT_LIB_DIR="${withval}"],)

AC_ARG_WITH(ciut-include,
            [AS_HELP_STRING([--with-ciut-include],[Directory to look for ciut headers])],
            [CIUT_INC_DIR="${withval}"],)
if test "x$CIUT_INC_DIR" != "x" ; then
  CFLAGS="-I${CIUT_INC_DIR} ${CFLAGS}"
  #SAVE_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="-I${CIUT_INC_DIR} $CXXFLAGS"
  AC_CHECK_HEADERS([ciut.h], [ LIBCIUTH='yes' ], AC_MSG_ERROR([Couldn't find ciut header ]), [])
  #CXXFLAGS="$SAVE_CXXFLAGS"
fi
if test "x$CIUT_LIB_DIR" != "x" ; then
  LDFLAGS="-L${CIUT_LIB_DIR} ${LDFLAGS}"
  #AC_CHECK_LIB([ciut], [ciut_main], [ LIBCIUTL='yes' ], AC_MSG_ERROR([Couldn't find ciut lib file ]), [-L${CIUT_LIB_DIR} -lciut])
fi

AX_VALGRIND_DFLT([sgcheck], [off])
AX_VALGRIND_CHECK
#AC_ARG_ENABLE([valgrind],
#  [AS_HELP_STRING([--enable-valgrind],
#    [use valgrind to test the test suite])],
#    [],
#    [enable_valgrind=no])
#AS_IF([test "$enable_valgrind" = yes],
#  [AC_PATH_PROG([VALGRIND], [valgrind])]
#  AS_IF([test -z "$VALGRIND"], [AC_MSG_ERROR([valgrind not found])])
#)
#AM_CONDITIONAL([USE_VALGRIND],[test '!' "$enable_valgrind" = no])


# debug
AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug],[Compile the debug version (default: disabled)]),
	[enable_debug=$enableval],
	[enable_debug=no])
AM_CONDITIONAL([DEBUG], [test $enable_debug = "yes"])

# coverage
AC_ARG_ENABLE([coverage],
    AS_HELP_STRING([--enable-coverage],[Enable lcov coverage report generation (default: disabled)]),
    [enable_coverage=$enableval],
    [enable_coverage=no])
if test "x$enable_coverage" = "xyes"; then
    AC_PATH_PROG([LCOV], [lcov], [], [$PATH:/bin:/usr/bin:/usr/local/bin])
    if test x$LCOV = x; then
        AC_MSG_ERROR([lcov not found])
    fi
    AC_PATH_PROG([GENHTML], [genhtml], [], [$PATH:/bin:/usr/bin:/usr/local/bin])
    if test x$GENHTML = x; then
        AC_MSG_ERROR([genhtml not found])
    fi

    COVERAGE_CFLAGS=
    COVERAGE_LDFLAGS=

    # llvm native coverage mode:
    #AS_CASE([${CC}], [*clang*],
    #  [AX_CHECK_COMPILE_FLAG([-fprofile-instr-generate],
    #  [COVERAGE_CFLAGS="$COVERAGE_CFLAGS -fprofile-instr-generate"])])
    #AS_CASE([${CC}], [*clang*],
    #  [AX_CHECK_COMPILE_FLAG([-fcoverage-mapping],
    #  [COVERAGE_CFLAGS="$COVERAGE_CFLAGS -fcoverage-mapping"])])

    # gcov or llvm compabililty coverage mode:
    AS_CASE([${CC}], [*],
      [AX_CHECK_COMPILE_FLAG([-fprofile-arcs],
      [COVERAGE_CFLAGS="$COVERAGE_CFLAGS -fprofile-arcs"])])
    AS_CASE([${CC}], [*],
      [AX_CHECK_COMPILE_FLAG([-ftest-coverage],
      [COVERAGE_CFLAGS="$COVERAGE_CFLAGS -ftest-coverage"])])
    AS_CASE([${CC}], [*],
      [AX_CHECK_COMPILE_FLAG([--coverage],
      [COVERAGE_CFLAGS="$COVERAGE_CFLAGS --coverage"])])

    AS_CASE([${CC}], [*],
      [AX_CHECK_LINK_FLAG([-fprofile-arcs],
      [COVERAGE_LDFLAGS="$COVERAGE_LDFLAGS -fprofile-arcs"])])
    AS_CASE([${CC}], [*],
      [AX_CHECK_LINK_FLAG([-ftest-coverage],
      [COVERAGE_LDFLAGS="$COVERAGE_LDFLAGS -ftest-coverage"])])
    AS_CASE([${CC}], [*],
      [AX_CHECK_LINK_FLAG([--coverage],
      [COVERAGE_LDFLAGS="$COVERAGE_LDFLAGS --coverage"])])

    AC_SUBST(COVERAGE_CFLAGS)
    AC_SUBST(COVERAGE_LDFLAGS)

    CFLAGS="${COVERAGE_CFLAGS} ${CFLAGS}"
    CXXFLAGS="${COVERAGE_CFLAGS} ${CXXFLAGS}"
    LDFLAGS="${COVERAGE_LDFLAGS} ${LDFLAGS}"

    AC_MSG_NOTICE([coverage enabled, adding "-g -O0" to CFLAGS])
    #CFLAGS="${CFLAGS} -g -O0"
    enable_debug=yes
    AC_SUBST(enable_debug)
fi
AM_CONDITIONAL([COVERAGE], [test x$enable_coverage = xyes])

if test "x$enable_debug" = "xyes"  || test "x$enable_coverage" = "xyes" ; then
  changequote({,})
  CFLAGS=`echo "$CFLAGS" | $SED -e 's/-O[0-9s]*//g'`
  CXXFLAGS=`echo "$CXXFLAGS" | $SED -e 's/-O[0-9s]*//g'`
  CPPFLAGS=`echo "$CPPFLAGS" | $SED -e 's/-O[0-9s]*//g'`
  LDFLAGS=`echo "$LDFLAGS" | $SED -e 's/-O[0-9s]*//g'`
  CFLAGS=`echo "$CFLAGS" | $SED -e 's/ -g //g'`
  CXXFLAGS=`echo "$CXXFLAGS" | $SED -e 's/ -g //g'`
  CPPFLAGS=`echo "$CPPFLAGS" | $SED -e 's/ -g //g'`
  LDFLAGS=`echo "$LDFLAGS" | $SED -e 's/ -g //g'`
  changequote([,])
  dnl add -O0 only if GCC or ICC is used
  if test "$GCC" = "yes" || test "$ICC" = "yes"; then
    CFLAGS="$CFLAGS -g -O0 -Wall"
    CXXFLAGS="$CXXFLAGS -g -O0 -Wall"
    CPPFLAGS="$CPPFLAGS -g -O0 -Wall"
    #LDFLAGS="$LDFLAGS -g -O0 -Wall"
  fi
fi

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([
    Makefile
    app/Makefile
    doc/Makefile
    doc/Doxyfile
    src/libgpsutils.pc
    src/Makefile
    tests/Makefile
    ])
AC_OUTPUT


#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(y2pmsh, 0.1, ludwig.nussel@suse.de)
AC_CONFIG_SRCDIR([src/build.cc])

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_CHECK_LIB([ncurses], [tigetstr], , [AC_MSG_ERROR([need ncurses-devel])])

# Checks for header files.
AC_CHECK_HEADERS([locale.h malloc.h stdint.h unistd.h])

AC_ARG_WITH(tecladir,
	    AC_HELP_STRING([--with-tecladir=VAL], [where libtecla is installed]))
if test "x$with_tecladir" != "x"; then
	tecladir="$with_tecladir"
else
	cd "$srcdir"
	tecladir="`pwd`/libtecla"
	cd -
fi
AC_SUBST(tecladir)

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([atexit setenv setlocale strcasecmp strdup strerror strtol])

if test ! -d $tecladir; then
	AC_MSG_ERROR([need libtecla (http://www.astro.caltech.edu/~mcs/tecla/index.html) to compile])
fi

AC_CONFIG_FILES([Makefile config/Makefile patches-9.0/Makefile src/Makefile])
AC_OUTPUT

AC_MSG_NOTICE([*** you need to "make -C $tecladir" first ***])

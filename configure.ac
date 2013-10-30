AC_INIT([libason], [0.1], [casey.dahlin@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability foreign parallel-tests])
AM_SILENT_RULES([yes])
AC_PROG_CC
m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR

## Check for GLFW ##
AC_ARG_WITH([asonq], [AS_HELP_STRING(
	     [--with-asonq],
	 [Build the asonq binary])
], [], [with_asonq=yes])

readline_LIBS=
readline_CFLAGS=

AS_IF([test "x$with_asonq" != xno], [
	AC_CHECK_LIB([readline], [readline], [
		readline_LIBS=-lreadline
	], [
		AC_MSG_FAILURE([--with-asonq requires readline])
	])
])

AC_ARG_VAR([readline_CFLAGS], [C compiler flags for readline])
AC_ARG_VAR([readline_LIBS], [linker flags for readline])

AM_CONDITIONAL([BUILD_ASONQ], [ test "x$with_asonq" != xno ])

AC_PATH_PROG([LEMON], [lemon], [no])
test "x$LEMON" = "xno" && AC_MSG_ERROR([Lemon parser generator not found])

AC_ARG_VAR([LEMON], [Lemon parser generator command])
LT_INIT
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	include/Makefile
	tests/Makefile
])
AC_OUTPUT

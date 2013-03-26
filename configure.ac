AC_INIT([libason], [0.1], [casey.dahlin@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign parallel-tests])
AM_SILENT_RULES([yes])
AC_PROG_CC
m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR
AC_PATH_PROG([LEMON], [lemon])
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

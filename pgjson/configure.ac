AC_PREREQ(2.59)
AC_INIT(FULL-PACKAGE-NAME, VERSION, BUG-REPORT-ADDRESS)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([include/pgjson_config.h])

AC_CONFIG_MACRO_DIR([m4])

dnl PKG_CHECK_MODULES([FREETYPE],
dnl 	freetype2,
dnl	[ft_enabled="yes"],
dnl	AC_MSG_WARN([*** Freetype2 not found!])
dnl	)
dnl PKG_CHECK_MODULES([LIBAGG],
dnl	libagg,
dnl	[agg_enabled="yes"],
dnl	AC_MSG_WARN([*** agg not found])
dnl	)

dnl Set some default flags
CPPFLAGS="$CPPFLAGS -Werror -Wimplicit"


AC_PROG_CC
AC_PROG_LEX
AC_PROG_YACC
AM_PROG_LIBTOOL
AX_LIB_POSTGRESQL

dnl Some autoconf bits adapted from PostGIS build
dnl
dnl Define PIC flags in PICFLAGS (note: this variable is set as part of libtool initialisation above)
dnl
PICFLAGS="$lt_prog_compiler_pic"
AC_SUBST([PICFLAGS])

dnl
dnl Version Information imported from Version.config 
dnl
PGJSON_MAJOR_VERSION=`cat Version.config | grep PGJSON_MAJOR_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
PGJSON_MINOR_VERSION=`cat Version.config | grep PGJSON_MINOR_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
PGJSON_MICRO_VERSION=`cat Version.config | grep PGJSON_MICRO_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`

AC_DEFINE_UNQUOTED([PGJSON_MAJOR_VERSION], ["$PGJSON_MAJOR_VERSION"], [PGJson major version])
AC_DEFINE_UNQUOTED([PGJSON_MINOR_VERSION], ["$PGJSON_MINOR_VERSION"], [PGJson minor version])
AC_DEFINE_UNQUOTED([PGJSON_MICRO_VERSION], ["$PGJSON_MICRO_VERSION"], [PGJson micro version])

AC_SUBST([PGJSON_MAJOR_VERSION])
AC_SUBST([PGJSON_MINOR_VERSION])
AC_SUBST([PGJSON_MICRO_VERSION])


dnl Implement opening stream against memory buffer on bsd (funopen) or linux fopencookie
AC_CHECK_FUNCS([funopen fopencookie])

AC_CONFIG_FILES([Makefile shared/Makefile pgplugin/Makefile])
AC_OUTPUT


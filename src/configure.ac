AC_INIT([wips], [1.0], [jeremiah.wala@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CXX

PKG_CHECK_MODULES([VIPS], [vips-cpp],
                  [], [AC_MSG_ERROR([vips-cpp pkg-config module not found])])

#AC_CHECK_HEADERS([vips/vips8.h], [],
#                 [AC_MSG_ERROR([libvips headers not found])])
#AC_CHECK_LIB([VIPS], [vips-cpp],
#            [], [AC_MSG_ERROR([libvips library not found])])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

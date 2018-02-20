AC_INIT([Vulkan Test Program], 1.0)
AM_INIT_AUTOMAKE
AC_PROG_CXX
AC_CONFIG_HEADERS([config.h])

# ----------------------------------------------------------
# DEBUG SUPPORT
# ----------------------------------------------------------

AC_ARG_ENABLE(debug,
        [  --enable-debug include debug symbols],,
        enable_debug=no)
if test "x$enable_debug" = "xyes"; then
	AC_MSG_WARN("Configuring with debugging options enabled!")
	CFLAGS="$CFLAGS -g3 -O0"
	CXXFLAGS="$CXXFLAGS -g3 -O0"
	PROG_DEFINES="-DENABLE_DEBUG=1"
	AC_SUBST(PROG_DEFINES)
else
	PROG_DEFINES="-DENABLE_DEBUG=0 -DNDEBUG"
	AC_SUBST(PROG_DEFINES)
fi

AM_CONDITIONAL([DEBUG], [test "x$enable_debug" = "xyes"])

PKG_CHECK_MODULES(PROG_DEPS, [vulkan glfw3 glm])
AC_SUBST(PROG_DEPS_CFLAGS)
AC_SUBST(PROG_DEPS_LIBS)

AC_CONFIG_FILES([
 Makefile
 src/Makefile
])
AC_OUTPUT

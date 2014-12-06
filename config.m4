dnl $Id$
dnl config.m4 for extension graphdat

PHP_ARG_ENABLE(graphdat, whether to enable Graphdat support,
			  [  --enable-graphdat           Enable Graphdat support])

PHP_ARG_WITH(libmsgpack, whether to use system msgpack,
			  [  --with-libmsgpack           Use system msgpack], no, no)

if test "$PHP_GRAPHDAT" != "no"; then
  dnl # --with-graphdat -> add include path

  #find . -name "*.c" -print 
  graphdat_sources=" \
    graphdat.c \
    src/cake.c \
    src/drupal7.c \
    src/joomla.c \
    src/magento.c \
    src/sockets.c \
    src/timers.c \
    src/wordpress.c \
    src/zendplugin.c"

  if test "$PHP_LIBMSGPACK" != "no"; then
    AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
    AC_MSG_CHECKING(msgpack version)
    if test -x "$PKG_CONFIG" && $PKG_CONFIG --exists msgpack; then
      LIBMSGPACK_INCLUDE=`$PKG_CONFIG msgpack --cflags`
      LIBMSGPACK_LIBRARY=`$PKG_CONFIG msgpack --libs`
      LIBMSGPACK_VERSION=`$PKG_CONFIG msgpack --modversion`
    fi

    if test -z "$LIBMSGPACK_VERSION"; then
      AC_MSG_RESULT(msgpack.pc not found)
      AC_CHECK_HEADERS([msgpack.h])
      PHP_CHECK_LIBRARY(msgpack, msgpack_version,
        [PHP_ADD_LIBRARY(msgpack, 1, GRAPHDAT_SHARED_LIBADD)],
        [AC_MSG_ERROR(msgpack library not found)])
    else
      AC_MSG_RESULT($LIBMSGPACK_VERSION)
      PHP_EVAL_INCLINE($LIBMSGPACK_INCLUDE)
      PHP_EVAL_LIBLINE($LIBMSGPACK_LIBRARY, GRAPHDAT_SHARED_LIBADD)
    fi
    PHP_NEW_EXTENSION(graphdat, $graphdat_sources, $ext_shared)
    PHP_SUBST(GRAPHDAT_SHARED_LIBADD)
  else
    graphdat_sources="$graphdat_sources \
      src/msgpack/objectc.c \
      src/msgpack/unpack.c \
      src/msgpack/version.c \
      src/msgpack/vrefbuffer.c \
      src/msgpack/zone.c"
    PHP_NEW_EXTENSION(graphdat, $graphdat_sources, $ext_shared)
    PHP_ADD_INCLUDE($ext_srcdir/src/msgpack)
    PHP_ADD_BUILD_DIR($ext_builddir/src/msgpack)
  fi

  PHP_ADD_INCLUDE($ext_srcdir/src)
  PHP_ADD_BUILD_DIR($ext_builddir/src)
fi

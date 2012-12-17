dnl $Id$
dnl config.m4 for extension graphdat

PHP_ARG_ENABLE(graphdat, whether to enable Graphdat support,
			  [  --enable-graphdat           Enable Graphdat support])

if test "$PHP_GRAPHDAT" != "no"; then
  dnl # --with-graphdat -> add include path

  #find . -name "*.c" -print 
  graphdat_sources=" \
    graphdat.c \
    src/cake.c \
    src/drupal7.c \
    src/joomla.c \
    src/magento.c \
    src/msgpack/objectc.c \
    src/msgpack/unpack.c \
    src/msgpack/version.c \
    src/msgpack/vrefbuffer.c \
    src/msgpack/zone.c \
    src/sockets.c \
    src/timers.c \
    src/wordpress.c \
    src/zendplugin.c"

  PHP_NEW_EXTENSION(graphdat, $graphdat_sources, $ext_shared)
  
  PHP_ADD_INCLUDE($ext_srcdir/src)
  PHP_ADD_INCLUDE($ext_srcdir/src/msgpack)
  PHP_ADD_BUILD_DIR($ext_builddir/src)
  PHP_ADD_BUILD_DIR($ext_builddir/src/msgpack)

fi

dnl $Id$
dnl config.m4 for extension graphdat

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(graphdat, for graphdat support,
dnl Make sure that the comment is aligned:
dnl [  --with-graphdat             Include graphdat support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(graphdat, whether to enable Graphdat support,
			  [  --enable-graphdat           Enable Graphdat support])

if test "$PHP_GRAPHDAT" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-graphdat -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/graphdat.h"  # you most likely want to change this
  dnl if test -r $PHP_GRAPHDAT/$SEARCH_FOR; then # path given as parameter
  dnl   GRAPHDAT_DIR=$PHP_GRAPHDAT
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for graphdat files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       GRAPHDAT_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$GRAPHDAT_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the graphdat distribution])
  dnl fi

  dnl # --with-graphdat -> add include path
  dnl PHP_ADD_INCLUDE($GRAPHDAT_DIR/include)

  dnl # --with-graphdat -> check for lib and symbol presence
  dnl LIBNAME=graphdat # you may want to change this
  dnl LIBSYMBOL=graphdat # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $GRAPHDAT_DIR/lib, GRAPHDAT_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_GRAPHDATLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong graphdat lib version or lib not found])
  dnl ],[
  dnl   -L$GRAPHDAT_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(GRAPHDAT_SHARED_LIBADD)

  PHP_NEW_EXTENSION(graphdat, graphdat.c, $ext_shared)
fi

dnl config.m4 for extension jpeg_optimizer

PHP_ARG_ENABLE(jpeg-optimizer, whether to enable jpeg-optimizer extension,
[  --enable-jpeg-optimizer          Enable jpeg optimization function with mozjpeg support], no)

if test -z "$PHP_JPEG_DIR"; then
  PHP_ARG_WITH(jpeg-dir, for the location of libjpeg,
  [  --with-jpeg-dir[=DIR]     GD: Set the path to libjpeg install prefix], no, no)
fi

if test -z "$PHP_IQA_DIR"; then
  PHP_ARG_WITH(iqa-dir, for the location of libiqa,
  [  --with-iqa-dir[=DIR]     GD: Set the path to libiqa install prefix], no, no)
fi


AC_DEFUN([PHP_JPEG_OPTIMIZER_JPEG_LIB],[
    for i in $PHP_JPEG_DIR /usr/local /usr /usr/lib/x86_64-linux-gnu/; do
      test -f $i/include/jpeglib.h && JPEG_DIR=$i && break
    done

    if test -z "$JPEG_DIR"; then
      AC_MSG_ERROR([jpeglib.h not found.])
    fi

    PHP_CHECK_LIBRARY(jpeg,jpeg_read_header,
    [
      PHP_ADD_INCLUDE($JPEG_DIR/include)
      PHP_ADD_LIBRARY_WITH_PATH(jpeg, $JPEG_DIR/$PHP_LIBDIR, JPEG_OPTIMIZER_SHARED_LIBADD)
    ],[
      AC_MSG_ERROR([Problem with libjpeg.(a|so). Please check config.log for more information.])
    ],[
      -L$JPEG_DIR/$PHP_LIBDIR
    ])

    if test -z "$JPEG_DIR"; then
        AC_MSG_RESULT([If configure fails try --with-jpeg-dir=<DIR>])
    fi
])

AC_DEFUN([PHP_JPEG_OPTIMIZER_IQA],[
    for i in $PHP_IQA_DIR /usr/local /usr /usr/lib/x86_64-linux-gnu/; do
      test -f $i/include/iqa.h && IQA_DIR=$i && break
    done

    if test -z "$IQA_DIR"; then
      AC_MSG_ERROR([iqa.h not found.])
    fi

    PHP_CHECK_LIBRARY(iqa,iqa_ssim,
    [
      PHP_ADD_INCLUDE($IQA_DIR/include)
      PHP_ADD_LIBRARY_WITH_PATH(iqa, $IQA_DIR/$PHP_LIBDIR, JPEG_OPTIMIZER_SHARED_LIBADD)
    ],[
      AC_MSG_ERROR([Problem with libiqa.(a|so). Please check config.log for more information.])
    ],[
      -L$IQA_DIR/$PHP_LIBDIR
    ])

    if test -z "$IQA_DIR"; then
        AC_MSG_RESULT([If configure fails try --with-iqa-dir=<DIR>])
    fi
])

if test "$PHP_JPEG_OPTIMIZER" != "no"; then
  PHP_JPEG_OPTIMIZER_JPEG_LIB
  PHP_JPEG_OPTIMIZER_IQA
  AC_DEFINE(HAVE_JPEG_OPTIMIZER, 1, [ Have jpeg optimization function support ])
  PHP_NEW_EXTENSION(jpeg_optimizer, edit.c util.c smallfry.c jpeg_optimizer.c, $ext_shared)
  PHP_SUBST(JPEG_OPTIMIZER_SHARED_LIBADD)
fi

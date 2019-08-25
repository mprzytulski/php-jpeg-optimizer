/* jpeg_optimizer extension for PHP */

#ifndef PHP_JPEG_OPTIMIZER_H
# define PHP_JPEG_OPTIMIZER_H

extern zend_module_entry jpeg_optimizer_module_entry;
# define phpext_jpeg_optimizer_ptr &jpeg_optimizer_module_entry

# define PHP_JPEG_OPTIMIZER_VERSION "0.1.0"

# if defined(ZTS) && defined(COMPILE_DL_JPEG_OPTIMIZER)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

const char *COMMENT = "Compressed by php jpeg-optimizer";

static enum filetype parseInputFiletype(const char *s);
static double getTargetFromPreset(long preset, long method);
static int parseSubsampling(const char *s);
FILE *openOutput(char *name);


#endif	/* PHP_JPEG_OPTIMIZER_H */


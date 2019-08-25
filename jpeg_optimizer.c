/* jpeg_optimizer extension for PHP */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_jpeg_optimizer.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "edit.h"
#include "iqa.h"
#include "smallfry.h"
#include "util.h"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

/* For compatibility with older PHP versions */
#ifndef ZEND_PARSE_PARAMETERS_NONE
#define ZEND_PARSE_PARAMETERS_NONE() \
	ZEND_PARSE_PARAMETERS_START(0, 0) \
	ZEND_PARSE_PARAMETERS_END()
#endif

#define DEBUG "/var/log/jpeg-optimizer.log"

#define JPEG_OPTIMIZER_QUALITY_LOW 1000
#define JPEG_OPTIMIZER_QUALITY_MEDIUM 1001
#define JPEG_OPTIMIZER_QUALITY_HIGH 1002
#define JPEG_OPTIMIZER_QUALITY_VERYHIGH 1003

#define JPEG_OPTIMIZER_SUBSAMPLING_DISABLED 3000
#define JPEG_OPTIMIZER_SUBSAMPLING_DEFAULT 3001
#define JPEG_OPTIMIZER_SUBSAMPLING_444 3002

#define JPEG_OPTIMIZER_METHOD_UNKNOWN 2000
#define JPEG_OPTIMIZER_METHOD_SSIM 2001
#define JPEG_OPTIMIZER_METHOD_MS_SSIM 2003
#define JPEG_OPTIMIZER_METHOD_SMALLFRY 2004
#define JPEG_OPTIMIZER_METHOD_MPE 2005

#define JPEG_OPTIMIZER_ERROR_INVALID_FILE_FORMAT 4001
#define JPEG_OPTIMIZER_ERROR_INVALID_COMPRESSION_LEVEL 4002
#define JPEG_OPTIMIZER_ERROR_INVALID_MINIMAL_QUALITY_LEVEL 4003
#define JPEG_OPTIMIZER_ERROR_INVALID_MAXIMUM_QUALITY_LEVEL 4004
#define JPEG_OPTIMIZER_ERROR_INVALID_SUBSAMPLING_MORE 4005
#define JPEG_OPTIMIZER_ERROR_FAILED_OPENING_OUTPUT_FILE 4006
#define JPEG_OPTIMIZER_ERROR_ZERO_SIZE_FILES 4007
#define JPEG_OPTIMIZER_ERROR_UNABLE_TO_DECODE 4008
#define JPEG_OPTIMIZER_ERROR_OUTPUT_FILE_LARGER_THAN_INPUT 4009
#define JPEG_OPTIMIZER_ERROR_MISSING_SOI_MARKER 4010
#define JPEG_OPTIMIZER_ERROR_MISSING_APP0_MARKER 4011
#define JPEG_OPTIMIZER_ERROR_UNABLE_TO_DECODE_SOURCE 4012


static double getTargetFromPreset(long preset, long method) {
    switch (method) {
        case JPEG_OPTIMIZER_METHOD_SSIM:
            switch (preset) {
                case JPEG_OPTIMIZER_QUALITY_LOW: return 0.999;
                case JPEG_OPTIMIZER_QUALITY_MEDIUM: return 0.9999;
                case JPEG_OPTIMIZER_QUALITY_HIGH: return 0.99995;
                case JPEG_OPTIMIZER_QUALITY_VERYHIGH: return 0.99999;
            }
            break;
        case JPEG_OPTIMIZER_METHOD_MS_SSIM:
            switch (preset) {
                case JPEG_OPTIMIZER_QUALITY_LOW: return 0.85;
                case JPEG_OPTIMIZER_QUALITY_MEDIUM: return 0.94;
                case JPEG_OPTIMIZER_QUALITY_HIGH: return 0.96;
                case JPEG_OPTIMIZER_QUALITY_VERYHIGH: return 0.98;
            }
            break;
        case JPEG_OPTIMIZER_METHOD_SMALLFRY:
            switch (preset) {
                case JPEG_OPTIMIZER_QUALITY_LOW: return 100.75;
                case JPEG_OPTIMIZER_QUALITY_MEDIUM: return 102.25;
                case JPEG_OPTIMIZER_QUALITY_HIGH: return 103.8;
                case JPEG_OPTIMIZER_QUALITY_VERYHIGH: return 105.5;
            }
            break;
        case JPEG_OPTIMIZER_METHOD_MPE:
            switch (preset) {
                case JPEG_OPTIMIZER_QUALITY_LOW: return 1.5;
                case JPEG_OPTIMIZER_QUALITY_MEDIUM: return 1.0;
                case JPEG_OPTIMIZER_QUALITY_HIGH: return 0.8;
                case JPEG_OPTIMIZER_QUALITY_VERYHIGH: return 0.6;
            }
            break;
    }
}

//// Open a file for writing
FILE *openOutput(char *name) {
    if (strcmp("-", name) == 0) {
#ifdef _WIN32
        setmode(fileno(stdout), O_BINARY);
#endif
        return stdout;
    } else {
        return fopen(name, "wb");
    }
}


/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(jpeg_optimizer)
{
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_QUALITY_LOW", JPEG_OPTIMIZER_QUALITY_LOW, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_QUALITY_MEDIUM", JPEG_OPTIMIZER_QUALITY_MEDIUM, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_QUALITY_HIGH", JPEG_OPTIMIZER_QUALITY_HIGH, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_QUALITY_VERYHIGH", JPEG_OPTIMIZER_QUALITY_VERYHIGH, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_SUBSAMPLING_DISABLED", JPEG_OPTIMIZER_SUBSAMPLING_DISABLED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_SUBSAMPLING_DEFAULT", JPEG_OPTIMIZER_SUBSAMPLING_DEFAULT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_SUBSAMPLING_444", JPEG_OPTIMIZER_SUBSAMPLING_444, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_METHOD_SSIM", JPEG_OPTIMIZER_METHOD_SSIM, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_METHOD_MS_SSIM", JPEG_OPTIMIZER_METHOD_MS_SSIM, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_METHOD_SMALLFRY", JPEG_OPTIMIZER_METHOD_SMALLFRY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_METHOD_MPE", JPEG_OPTIMIZER_METHOD_MPE, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_ERROR_INVALID_FILE_FORMAT", JPEG_OPTIMIZER_ERROR_INVALID_FILE_FORMAT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_ERROR_INVALID_COMPRESSION_LEVEL", JPEG_OPTIMIZER_ERROR_INVALID_COMPRESSION_LEVEL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_ERROR_INVALID_MINIMAL_QUALITY_LEVEL", JPEG_OPTIMIZER_ERROR_INVALID_MINIMAL_QUALITY_LEVEL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_ERROR_INVALID_MAXIMUM_QUALITY_LEVEL", JPEG_OPTIMIZER_ERROR_INVALID_MAXIMUM_QUALITY_LEVEL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_ERROR_INVALID_SUBSAMPLING_MORE", JPEG_OPTIMIZER_ERROR_INVALID_SUBSAMPLING_MORE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_ERROR_FAILED_OPENING_OUTPUT_FILE", JPEG_OPTIMIZER_ERROR_FAILED_OPENING_OUTPUT_FILE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_ERROR_ZERO_SIZE_FILES", JPEG_OPTIMIZER_ERROR_ZERO_SIZE_FILES, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_ERROR_UNABLE_TO_DECODE", JPEG_OPTIMIZER_ERROR_UNABLE_TO_DECODE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_ERROR_OUTPUT_FILE_LARGER_THAN_INPUT", JPEG_OPTIMIZER_ERROR_OUTPUT_FILE_LARGER_THAN_INPUT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_ERROR_MISSING_SOI_MARKER", JPEG_OPTIMIZER_ERROR_MISSING_SOI_MARKER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JPEG_OPTIMIZER_ERROR_MISSING_APP0_MARKER", JPEG_OPTIMIZER_ERROR_MISSING_APP0_MARKER, CONST_CS | CONST_PERSISTENT);


    return SUCCESS;
}
/* }}} */

void info(const char *format, ...) {
#ifdef DEBUG
    FILE * log;
    log = fopen(DEBUG, "a+");
    va_list argptr;
    va_start(argptr, format);
    vfprintf(log, format, argptr);
    va_end(argptr);
    fclose(log);
#endif
}

/* {{{ string jpeg_optimize( [ string $var ] )
 */
PHP_FUNCTION(jpeg_optimize)
{
    zend_string *inputPath;
    zend_string *outputPath;

    zend_long method = JPEG_OPTIMIZER_METHOD_SSIM;

    // Number of binary search steps
    zend_long attempts = 6;
    double target = 0;
    zend_long preset = JPEG_OPTIMIZER_QUALITY_MEDIUM;

    // Min/max JPEG quality
    zend_long jpegMin = 40;
    zend_long jpegMax = 95;

    // Strip metadata from the file?
    zend_bool strip = FALSE;
    zend_bool progressive = TRUE;

    double defishStrength = 0.0;
    double defishZoom = 1.0;
    enum filetype inputFiletype = FILETYPE_AUTO;
    zend_bool accurate = FALSE;
    zend_long subsample = JPEG_OPTIMIZER_SUBSAMPLING_DEFAULT;

    zend_string *retval;

    ZEND_PARSE_PARAMETERS_START(2, 14)
    Z_PARAM_STR(inputPath)
    Z_PARAM_STR(outputPath)

    Z_PARAM_OPTIONAL
    Z_PARAM_LONG(preset)
    Z_PARAM_LONG(method)
    Z_PARAM_LONG(jpegMin)
    Z_PARAM_LONG(jpegMax)
    Z_PARAM_LONG(attempts)
    Z_PARAM_BOOL(accurate)
    Z_PARAM_BOOL(strip)
    Z_PARAM_BOOL(progressive)
    Z_PARAM_DOUBLE(defishStrength)
    Z_PARAM_DOUBLE(defishZoom)
    Z_PARAM_LONG(subsample)
    ZEND_PARSE_PARAMETERS_END();

    retval = zend_strpprintf(0, "%s -> %s {preset: %d, target: %f, min: %d, max: %d, attempts: %d, accurate: %d, method: %d, strip: %d, strength: %f, zoom: %f, subsample: %d}",
    ZSTR_VAL(inputPath), ZSTR_VAL(outputPath), preset, target, jpegMin, jpegMax, attempts, accurate, method, strip, defishStrength, defishZoom, subsample);

    if (preset < JPEG_OPTIMIZER_QUALITY_LOW || preset > JPEG_OPTIMIZER_QUALITY_VERYHIGH) {
        php_error_docref(NULL, E_WARNING, "Invalid compression level (%d). Check JPEG_OPTIMIZER_QUALITY_* consts. ", preset);
        RETURN_LONG(JPEG_OPTIMIZER_ERROR_INVALID_COMPRESSION_LEVEL);
    }

    if (jpegMin <= 0 || jpegMin > 99) {
        php_error_docref(NULL, E_WARNING, "Invalid minimal value for jpeg compression level (%d). Value must be between 0 and 99", jpegMin);
        RETURN_LONG(JPEG_OPTIMIZER_ERROR_INVALID_MINIMAL_QUALITY_LEVEL);
    }

    if (jpegMax < 1 || jpegMax > 100) {
        php_error_docref(NULL, E_WARNING, "Invalid max value for jpeg compression level (%d). Value must be between 1 and 100", jpegMax);
        RETURN_LONG(JPEG_OPTIMIZER_ERROR_INVALID_MAXIMUM_QUALITY_LEVEL);
    }

    if (subsample < JPEG_OPTIMIZER_SUBSAMPLING_DISABLED || subsample > JPEG_OPTIMIZER_SUBSAMPLING_444) {
        php_error_docref(NULL, E_WARNING, "Invalid sub sampling method (%d). Check JPEG_OPTIMIZER_SUBSAMPLING_* consts. ", subsample);
        RETURN_LONG(JPEG_OPTIMIZER_ERROR_INVALID_SUBSAMPLING_MORE);
    }

    target = getTargetFromPreset(preset, method);

    int opt, longind = 0;
    unsigned char *buf;
    long bufSize = 0;
    unsigned char *original;
    long originalSize = 0;
    unsigned char *originalGray = NULL;
    long originalGraySize = 0;
    unsigned char *compressed = NULL;
    unsigned long compressedSize = 0;
    unsigned char *compressedGray;
    long compressedGraySize = 0;
    unsigned char *tmpImage;
    int width, height;
    unsigned char *metaBuf;
    unsigned int metaSize = 0;
    FILE *file;

    /* Read the input into a buffer. */
    bufSize = readFile(ZSTR_VAL(inputPath), (void **) &buf);

    /* Detect input file type. */
    if (inputFiletype == FILETYPE_AUTO)
    inputFiletype = detectFiletypeFromBuffer(buf, bufSize);

    /*
     * Read original image and decode. We need the raw buffer contents and its
     * size to obtain meta data and the original file size later.
     */
    originalSize = decodeFileFromBuffer(buf, bufSize, &original, inputFiletype, &width, &height, JCS_RGB);
    if (!originalSize) {
        php_error_docref(NULL, E_WARNING, "Invalid input file %s ", ZSTR_VAL(inputPath));
        RETURN_LONG(FALSE);
    }

    if (defishStrength) {
        tmpImage = malloc(width * height * 3);
        defish(original, tmpImage, width, height, 3, defishStrength, defishZoom);
        free(original);
        original = tmpImage;
    }

    // Convert RGB input into Y
    originalGraySize = grayscale(original, &originalGray, width, height);

    if (inputFiletype == FILETYPE_JPEG) {
        // Read metadata (EXIF / IPTC / XMP tags)
        if (getMetadata(buf, bufSize, &metaBuf, &metaSize, COMMENT)) {
            php_error_docref(NULL, E_NOTICE, "File already processed by php jpeg optimizer %s ", ZSTR_VAL(inputPath));
            file = openOutput(ZSTR_VAL(outputPath));
            if (file == NULL) {
                php_error_docref(NULL, E_WARNING, "Could not open output file %s ", ZSTR_VAL(outputPath));
                RETURN_LONG(JPEG_OPTIMIZER_ERROR_FAILED_OPENING_OUTPUT_FILE);
            }

            fwrite(buf, bufSize, 1, file);
            fclose(file);
            free(buf);

            RETURN_BOOL(TRUE);
        }
    }

    if (strip) {
        // Pretend we have no metadata
        metaSize = 0;
    }

    if (!originalSize || !originalGraySize) {
        php_error_docref(NULL, E_WARNING, "Unable to process file %s ", ZSTR_VAL(outputPath));
        RETURN_LONG(JPEG_OPTIMIZER_ERROR_ZERO_SIZE_FILES);
    }

    // Do a binary search to find the optimal encoding quality for the
    // given target SSIM value.
    int min = jpegMin, max = jpegMax;
    for (int attempt = attempts - 1; attempt >= 0; --attempt) {
        float metric;
        int quality = min + (max - min) / 2;

        /* Terminate early once bisection interval is a singleton. */
        if (min == max)
            attempt = 0;

        int progressive = attempt ? 0 : (progressive ? 1 : 0);
        int optimize = accurate ? 1 : (attempt ? 0 : 1);

        // Recompress to a new quality level, without optimizations (for speed)
        compressedSize = encodeJpeg(&compressed, original, width, height, JCS_RGB, quality, progressive, optimize, subsample);

        // Load compressed luma for quality comparison
        compressedGraySize = decodeJpeg(compressed, compressedSize, &compressedGray, &width, &height, JCS_GRAYSCALE);

        if (!compressedGraySize) {
            php_error_docref(NULL, E_WARNING, "Unable to decode file that was just encoded");
            RETURN_LONG(JPEG_OPTIMIZER_ERROR_UNABLE_TO_DECODE);
        }

        if (!attempt) {
            info("Final optimized ");
        }

        // Measure quality difference
        switch (method) {
            case JPEG_OPTIMIZER_METHOD_MS_SSIM:
                metric = iqa_ms_ssim(originalGray, compressedGray, width, height, width, 0);
                info("ms-ssim");
                break;
            case JPEG_OPTIMIZER_METHOD_SMALLFRY:
                metric = smallfry_metric(originalGray, compressedGray, width, height);
                info("smallfry");
                break;
            case JPEG_OPTIMIZER_METHOD_MPE:
                metric = meanPixelError(originalGray, compressedGray, width, height, 1);
                info("mpe");
                break;
            case JPEG_OPTIMIZER_METHOD_SSIM: default:
                metric = iqa_ssim(originalGray, compressedGray, width, height, width, 0, 0);
                info("ssim");
                break;
        }

        if (attempt) {
            info(" at q=%i (%i - %i): %f\n", quality, min, max, metric);
        } else {
            info(" at q=%i: %f\n", quality, metric);
        }

        if (metric < target) {
            if (compressedSize >= bufSize) {
                free(compressed);
                free(compressedGray);

                info("Output file would be larger than input!\n");
                file = openOutput(ZSTR_VAL(outputPath));
                if (file == NULL) {
                    php_error_docref(NULL, E_WARNING, "Could not open output file: %s", outputPath);
                    RETURN_LONG(JPEG_OPTIMIZER_ERROR_FAILED_OPENING_OUTPUT_FILE);
                }

                fwrite(buf, bufSize, 1, file);
                fclose(file);

                free(buf);

                RETURN_BOOL(TRUE);
            }

            switch (method) {
                case JPEG_OPTIMIZER_METHOD_SSIM: case JPEG_OPTIMIZER_METHOD_MS_SSIM: case JPEG_OPTIMIZER_METHOD_SMALLFRY:
                    // Too distorted, increase quality
                    min = MIN(quality + 1, max);
                    break;
                case JPEG_OPTIMIZER_METHOD_MPE:
                    // Higher than required, decrease quality
                    max = MAX(quality - 1, min);
                    break;
            }
        } else {
            switch (method) {
                case JPEG_OPTIMIZER_METHOD_SSIM: case JPEG_OPTIMIZER_METHOD_MS_SSIM: case JPEG_OPTIMIZER_METHOD_SMALLFRY:
                    // Higher than required, decrease quality
                    max = MAX(quality - 1, min);
                    break;
                case JPEG_OPTIMIZER_METHOD_MPE:
                    // Too distorted, increase quality
                    min = MIN(quality + 1, max);
                    break;
            }
        }

        // If we aren't done yet, then free the image data
        if (attempt) {
            free(compressed);
            free(compressedGray);
        }
    }

    free(buf);

    // Calculate and show savings, if any
    int percent = (compressedSize + metaSize) * 100 / bufSize;
    unsigned long saved = (bufSize > compressedSize) ? bufSize - compressedSize - metaSize : 0;
    info("New size is %i%% of original (saved %lu kb)\n", percent, saved / 1024);

    if (compressedSize >= bufSize) {
        php_error_docref(NULL, E_NOTICE, "Output file is larger than input, aborting!", outputPath);
        RETURN_LONG(JPEG_OPTIMIZER_ERROR_OUTPUT_FILE_LARGER_THAN_INPUT);
    }

    // Open output file for writing
    file = openOutput(ZSTR_VAL(outputPath));
    if (file == NULL) {
        php_error_docref(NULL, E_WARNING, "Could not open output file: %s", outputPath);
        RETURN_LONG(JPEG_OPTIMIZER_ERROR_FAILED_OPENING_OUTPUT_FILE);
    }

    /* Check that the metadata starts with a SOI marker. */
    if (!checkJpegMagic(compressed, compressedSize)) {
        php_error_docref(NULL, E_WARNING, "Missing SOI marker, aborting!");
        RETURN_LONG(JPEG_OPTIMIZER_ERROR_MISSING_SOI_MARKER);
    }

    /* Make sure APP0 is recorded immediately after the SOI marker. */
    if (compressed[2] != 0xff || compressed[3] != 0xe0) {
        php_error_docref(NULL, E_WARNING, "Missing APP0 marker, aborting!");
        RETURN_LONG(JPEG_OPTIMIZER_ERROR_MISSING_APP0_MARKER);
    }

    /* Write SOI marker and APP0 metadata to the output file. */
    int app0_len = (compressed[4] << 8) + compressed[5];
    fwrite(compressed, 4 + app0_len, 1, file);

    /*
     * Write comment (COM metadata) so we know not to reprocess this file in
     * the future if it gets passed in again.
     */
    fputc(0xff, file);
    fputc(0xfe, file);
    fputc(0x00, file);
    fputc(strlen(COMMENT) + 2, file);
    fwrite(COMMENT, strlen(COMMENT), 1, file);

    /* Write additional metadata markers. */
    if (inputFiletype == FILETYPE_JPEG && !strip) {
        fwrite(metaBuf, metaSize, 1, file);
    }

    /* Write image data. */
    fwrite(compressed + 4 + app0_len, compressedSize - 4 - app0_len, 1, file);
    fclose(file);

    if (inputFiletype == FILETYPE_JPEG && !strip) {
        free(metaBuf);
    }

    free(compressed);
    free(original);
    free(originalGray);


    RETURN_BOOL(TRUE);
}
/* }}}*/

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(jpeg_optimizer)
{
#if defined(ZTS) && defined(COMPILE_DL_JPEG_OPTIMIZER)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(jpeg_optimizer)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "jpeg_optimizer support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ arginfo
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_jpeg_optimize, 0, 0, 2)
    ZEND_ARG_INFO(0, source_file)
    ZEND_ARG_INFO(0, ouput_file)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ jpeg_optimizer_functions[]
 */
static const zend_function_entry jpeg_optimizer_functions[] = {
	PHP_FE(jpeg_optimize,		arginfo_jpeg_optimize)
	PHP_FE_END
};
/* }}} */

/* {{{ jpeg_optimizer_module_entry
 */
zend_module_entry jpeg_optimizer_module_entry = {
	STANDARD_MODULE_HEADER,
	"jpeg_optimizer",					/* Extension name */
	jpeg_optimizer_functions,			/* zend_function_entry */
    PHP_MINIT(jpeg_optimizer),/* PHP_MINIT - Module initialization */
	NULL,							/* PHP_MSHUTDOWN - Module shutdown */
	PHP_RINIT(jpeg_optimizer),			/* PHP_RINIT - Request initialization */
	NULL,							/* PHP_RSHUTDOWN - Request shutdown */
	PHP_MINFO(jpeg_optimizer),			/* PHP_MINFO - Module info */
	PHP_JPEG_OPTIMIZER_VERSION,		/* Version */
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_JPEG_OPTIMIZER
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(jpeg_optimizer)
#endif


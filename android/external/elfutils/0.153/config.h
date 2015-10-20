/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Should ar and ranlib use -D behavior by default? */
#define DEFAULT_AR_DETERMINISTIC false

/* $libdir subdirectory containing libebl modules. */
#define LIBEBL_SUBDIR "elfutils"

/* Identifier for modules in the build. */
#define MODVERSION "Build on bccheng.mtv.corp.google.com 2013-11-20T14:06:10-0800"

/* Define to 32 or 64 if a specific implementation is wanted. */
/* #undef NATIVE_ELF */

/* Name of package */
#define PACKAGE "elfutils"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "http://bugzilla.redhat.com/bugzilla/"

/* Define to the full name of this package. */
#define PACKAGE_NAME "Red Hat elfutils"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "Red Hat elfutils 0.153"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "elfutils"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.153"

/* Support bzip2 decompression via -lbz2. */
/* #undef USE_BZLIB */

/* Defined if demangling is enabled */
#define USE_DEMANGLE 1

/* Defined if libraries should be thread-safe. */
/* #undef USE_LOCKS */

/* Support LZMA (xz) decompression via -llzma. */
/* #undef USE_LZMA */

/* Support gzip decompression via -lz. */
/* #undef USE_ZLIB */

/* Version number of package */
#define VERSION "0.153"

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
#define YYTEXT_POINTER 1

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

#include <eu-config.h>

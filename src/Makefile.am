
AM_LDFLAGS=
AM_CFLAGS=
EXTRA_DIST=

DEFS+= \
    -Isrc \
    `getconf LFS_CFLAGS` \
    `getconf LFS64_CFLAGS` \
    -D_GNU_SOURCE \
    -D_FILE_OFFSET_BITS=64 \
    $(NULL)


AM_CFLAGS+= \
    -I$(top_builddir)/include/ \
    -I$(top_builddir)/src/ \
    -I$(top_srcdir)/include/ \
    -I$(top_srcdir)/src/ \
    $(NULL)

AM_LDFLAGS += \
    -L$(top_builddir)/src/ \
    $(NULL)

if DEBUG
# use "valgrind --tool=memcheck --leak-check=yes" to check memory leak, MemWatch will drag the program.
#DEFS+=-DMEMWATCH
DEFS+= -DDEBUG=1
AM_CFLAGS+=-g -Wall

else
AM_CFLAGS+=-O3 -Wall
endif
################################################################################

SRC_BASE= \
    ubloxconn.c \
    ubloxcstr.c \
    $(NULL)

include_HEADERS = \
    ubloxconn.h \
    ubloxcstr.h \
    $(NULL)

noinst_HEADERS= \
    $(NULL)

EXTRA_DIST += \
    libgpsutils.pc.in \
    $(NULL)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpsutils.pc


lib_LTLIBRARIES=libgpsutils.la

libgpsutils_la_SOURCES=$(SRC_BASE)

libgpsutils_la_CFLAGS=$(AM_CFLAGS)
#libgpsutils_la_CPPFLAGS=$(libgpsutils_la_CFLAGS)
libgpsutils_la_LDFLAGS=-lz



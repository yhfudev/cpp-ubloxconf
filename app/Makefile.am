
AM_LDFLAGS=
AM_CFLAGS=

DEFS += \
    `getconf LFS_CFLAGS` \
    `getconf LFS64_CFLAGS` \
    -D_GNU_SOURCE \
    -D_FILE_OFFSET_BITS=64 \
    -DHAVE_MMAP64=1 \
    -DUSE_EDIO24_SERVER=1 \
    $(NULL)

AM_CFLAGS += \
    -I$(top_srcdir)/src/ \
    -I$(top_srcdir)/include/ \
    -I$(top_builddir)/ \
    -I$(top_builddir)/src/ \
    -I$(top_builddir)/include/ \
    $(NULL)

AM_LDFLAGS += \
    -L$(top_builddir)/src/ \
    `getconf LFS_LDFLAGS` \
    `getconf LFS64_LDFLAGS` \
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

include_HEADERS = \
    $(NULL)

noinst_HEADERS= \
    $(NULL)

ubloxconf_SOURCES= \
    ubloxconf.c \
    $(NULL)

ubloxconf_LDADD = $(top_builddir)/src/libgpsutils.la
ubloxconf_CFLAGS=$(AM_CFLAGS) $(libgpsutils_la_CFLAGS)
ubloxconf_LDFLAGS=$(AM_LDFLAGS) -lgpsutils $(libgpsutils_la_LDFLAGS)


bin_PROGRAMS=ubloxconf
#noinst_PROGRAMS=ubloxconf



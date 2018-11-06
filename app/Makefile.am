
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


#AM_CFLAGS += `pkg-config --cflags libuv`
#AM_LDFLAGS += `pkg-config --libs libuv`
AM_CFLAGS += $(LIBUV_CFLAGS)
AM_LDFLAGS+= $(LIBUV_LDFLAGS) $(LIBUV_LIBS)


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
    utils.c \
    ubloxconf.c \
    $(NULL)

ubloxconf_LDADD = $(top_builddir)/src/libgpsutils.la -luv -ldl
ubloxconf_CFLAGS=$(AM_CFLAGS) $(libgpsutils_la_CFLAGS)
ubloxconf_LDFLAGS=$(AM_LDFLAGS) -lgpsutils $(libgpsutils_la_LDFLAGS)


bin_PROGRAMS=ubloxconf
#noinst_PROGRAMS=ubloxconf



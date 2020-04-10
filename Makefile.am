SUBDIRS =

if BUILD_TRACER
SUBDIRS += tracer
endif

if BUILD_T_INFINITY
SUBDIRS += t-infinity
endif

SUBDIRS += src

DIST_SUBDIRS = @subdirs@ src

#ACLOCAL_AMFLAGS = -I config -I $(build_tools)/aclocal @ACLOCAL_AMFLAGS@
ACLOCAL_AMFLAGS = -I config @ACLOCAL_AMFLAGS@

EXTRA_DIST = bootstrap

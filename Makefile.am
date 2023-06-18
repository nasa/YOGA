SUBDIRS = ddata tracer

if BUILD_MPI
SUBDIRS += MessagePasser parfait t-infinity
if BUILD_PLUGIN
SUBDIRS += plugin-parfait
endif
if BUILD_NANOFLANN
SUBDIRS += yoga
endif
endif

DIST_SUBDIRS = ${subdirs} \
	ddata tracer MessagePasser parfait t-infinity plugin-parfait yoga

ACLOCAL_AMFLAGS = -I config @ACLOCAL_AMFLAGS@

EXTRA_DIST = bootstrap

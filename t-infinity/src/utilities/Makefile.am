SUBDIRS = DistanceCalculator

bin_PROGRAMS = inf

lib_LTLIBRARIES = \
	inf_SubCommand_core_cartmesh.la \
	inf_SubCommand_core_csv-to-snap.la \
	inf_SubCommand_core_distance.la \
	inf_SubCommand_core_extensions.la \
	inf_SubCommand_core_extrude.la \
	inf_SubCommand_core_fix.la \
	inf_SubCommand_core_interpolate.la \
	inf_SubCommand_core_plot.la \
	inf_SubCommand_core_plugins.la \
	inf_SubCommand_core_sampling.la \
	inf_SubCommand_core_snap.la \
	inf_SubCommand_core_transform.la \
	inf_SubCommand_core_validate.la

AM_CXXFLAGS = \
	@mpi_include@ \
	@MessagePasser_include@ \
	@parfait_include@ \
	@tracer_include@ \
	@t_infinity_include@ \
	@t_infinity_interfaces_include@ \
	-I$(srcdir)

AM_LDFLAGS = -module -avoid-version -no-undefined
LIBADD = \
	@parfait_path@/parfait/libparfait.la \
	@t_infinity_path@/src/t-infinity/libinfinity.la \
	@tracer_path@/include/libtracer.la \
	@mpixx_ldadd@

inf_CXXFLAGS = -DDRIVER_PREFIX=\"inf\" \
	@mpi_include@ \
	@MessagePasser_include@ \
	@parfait_include@ \
	@tracer_include@ \
	@t_infinity_include@ \
	@t_infinity_interfaces_include@ \
	-I$(srcdir)
inf_SOURCES = inf.cpp
inf_LDADD = \
	@parfait_path@/parfait/libparfait.la \
	@t_infinity_path@/src/t-infinity/libinfinity.la \
	@tracer_path@/include/libtracer.la \
	@mpixx_ldadd@

inf_SubCommand_core_cartmesh_la_SOURCES = CreateCartMeshCommand.cpp
inf_SubCommand_core_cartmesh_la_LIBADD = $(LIBADD)

inf_SubCommand_core_csv_to_snap_la_SOURCES = CsvToSnapCommand.cpp
inf_SubCommand_core_csv_to_snap_la_LIBADD = $(LIBADD)

inf_SubCommand_core_distance_la_SOURCES = DistanceCommand.cpp
inf_SubCommand_core_distance_la_LIBADD = $(LIBADD)

inf_SubCommand_core_extensions_la_CXXFLAGS = -DDRIVER_PREFIX=\"inf\" \
	@mpi_include@ \
	@MessagePasser_include@ \
	@parfait_include@ \
	@tracer_include@ \
	@t_infinity_include@ \
	@t_infinity_interfaces_include@ \
	-I$(srcdir)
inf_SubCommand_core_extensions_la_SOURCES = ExtensionsCommand.cpp
inf_SubCommand_core_extensions_la_LIBADD = $(LIBADD)

inf_SubCommand_core_extrude_la_SOURCES = ExtrudeCommand.cpp
inf_SubCommand_core_extrude_la_LIBADD = $(LIBADD)

inf_SubCommand_core_fix_la_SOURCES = FixCommand.cpp
inf_SubCommand_core_fix_la_LIBADD = $(LIBADD)

inf_SubCommand_core_interpolate_la_SOURCES = InterpolateCommand.cpp
inf_SubCommand_core_interpolate_la_LIBADD = $(LIBADD)

inf_SubCommand_core_plot_la_SOURCES = PlotCommand.cpp
inf_SubCommand_core_plot_la_LIBADD = $(LIBADD)

inf_SubCommand_core_plugins_la_SOURCES = ListPluginsCommand.cpp
inf_SubCommand_core_plugins_la_LIBADD = $(LIBADD)

inf_SubCommand_core_sampling_la_SOURCES = SamplingCommand.cpp
inf_SubCommand_core_sampling_la_LIBADD = $(LIBADD)

inf_SubCommand_core_snap_la_SOURCES = SnapCommand.cpp
inf_SubCommand_core_snap_la_LIBADD = $(LIBADD)

inf_SubCommand_core_transform_la_SOURCES = TransformCommand.cpp
inf_SubCommand_core_transform_la_LIBADD = $(LIBADD)

inf_SubCommand_core_validate_la_SOURCES = ValidateCommand.cpp
inf_SubCommand_core_validate_la_LIBADD = $(LIBADD)

install-exec-hook:
	$(RM) -f \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_cartmesh.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_csv-to-snap.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_distance.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_extensions.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_extrude.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_fix.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_interpolate.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_plot.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_plugins.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_sampling.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_snap.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_snap-merge.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_transform.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_validate.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_cartmesh.a \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_csv-to-snap.a \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_distance.a \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_extensions.a \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_extrude.a \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_fix.a \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_interpolate.a \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_plot.a \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_plugins.a \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_sampling.a \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_snap.a \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_snap-merge.a \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_transform.a \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_validate.a

uninstall-hook:
	$(RM) -f \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_cartmesh.so \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_csv-to-snap.so \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_distance.so \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_extensions.so \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_extrude.so \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_fix.so \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_interpolate.so \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_plot.so \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_plugins.so \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_sampling.so \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_snap.so \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_snap-merge.so \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_transform.so \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_validate.so


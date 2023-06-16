bin_PROGRAMS = yoga

lib_LTLIBRARIES = \
	yoga_SubCommand_core_extensions.la \
	yoga_SubCommand_core_composite-rotor.la \
	yoga_SubCommand_core_make-composite.la \
	yoga_SubCommand_core_assemble.la \
	yoga_SubCommand_core_show-syntax.la \
	yoga_SubCommand_core_verify.la \
	yoga_SubCommand_core_split-csv.la \
	yoga_SubCommand_core_check-dcif.la \
	yoga_SubCommand_core_plot-dcif.la \
	yoga_SubCommand_core_check-syntax.la \
	yoga_SubCommand_experimental_fix-orphan.la \
	yoga_SubCommand_experimental_rotate-metric.la \
	inf_SubCommand_experimental_cube-sampling.la

AM_CXXFLAGS = \
	-DDRIVER_PREFIX=\"yoga\" \
	@mpi_include@ \
	@MessagePasser_include@ \
	@parfait_include@ \
	@tracer_include@ \
	@t_infinity_include@ \
	@ddata_include@ \
	@t_infinity_interfaces_include@ \
	-I$(srcdir)/../src \
	-I$(srcdir)

AM_LDFLAGS = -module -avoid-version -no-undefined
LIBADD = \
	../src/libPartitionFileMeshLoader.la \
	../src/libYogaPlugin.la \
	../src/libyoga.la \
	@parfait_path@/parfait/libparfait.la \
	@t_infinity_path@/src/t-infinity/libinfinity.la \
	@tracer_path@/include/libtracer.la \
	@mpixx_ldadd@

yoga_SOURCES = yoga.cpp
yoga_LDADD = \
	../src/libyoga.la \
	@parfait_path@/parfait/libparfait.la \
	@t_infinity_path@/src/t-infinity/libinfinity.la \
	@tracer_path@/include/libtracer.la \
	@mpixx_ldadd@

yoga_SubCommand_core_extensions_la_SOURCES = ExtensionsCommand.cpp
yoga_SubCommand_core_extensions_la_LIBADD = $(LIBADD)

yoga_SubCommand_core_composite_rotor_la_LIBADD = $(LIBADD)
yoga_SubCommand_core_composite_rotor_la_SOURCES = CompositeRotorCommand.cpp

yoga_SubCommand_core_make_composite_la_SOURCES = MakeCompositeCommand.cpp
yoga_SubCommand_core_make_composite_la_LIBADD = $(LIBADD)

yoga_SubCommand_core_assemble_la_SOURCES = \
	AssemblyHelpers.h \
	AssembleCommand.cpp
yoga_SubCommand_core_assemble_la_LIBADD = $(LIBADD)

yoga_SubCommand_core_show_syntax_la_SOURCES = InputSyntaxCommand.cpp
yoga_SubCommand_core_show_syntax_la_LIBADD = $(LIBADD)

yoga_SubCommand_core_verify_la_SOURCES = VerifyNodeStatusCommand.cpp
yoga_SubCommand_core_verify_la_LIBADD = $(LIBADD)

yoga_SubCommand_core_split_csv_la_SOURCES = CompositeCsvSplitterCommand.cpp
yoga_SubCommand_core_split_csv_la_LIBADD = $(LIBADD)

yoga_SubCommand_core_check_dcif_la_SOURCES = CheckDcifCommand.cpp
yoga_SubCommand_core_check_dcif_la_LIBADD = $(LIBADD)

yoga_SubCommand_core_plot_dcif_la_SOURCES = PlotDciCommand.cpp
yoga_SubCommand_core_plot_dcif_la_LIBADD = $(LIBADD)

yoga_SubCommand_core_check_syntax_la_SOURCES = CheckSyntaxCommand.cpp
yoga_SubCommand_core_check_syntax_la_LIBADD = $(LIBADD)

yoga_SubCommand_experimental_fix_orphan_la_SOURCES = \
	AssemblyHelpers.h \
	FixOrphanCommand.cpp 
yoga_SubCommand_experimental_fix_orphan_la_LIBADD = $(LIBADD)

yoga_SubCommand_experimental_rotate_metric_la_SOURCES = RotateMetricCommand.cpp
yoga_SubCommand_experimental_rotate_metric_la_LIBADD = $(LIBADD)

inf_SubCommand_experimental_cube_sampling_la_SOURCES = \
	CubeSampling.h \
	CubeSamplingCommand.cpp
inf_SubCommand_experimental_cube_sampling_la_LIBADD = $(LIBADD)

install-exec-hook:
	$(RM) -f \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_extensions.la \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_composite-rotor.la \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_make-composite.la \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_assemble.la \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_show-syntax.la \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_verify.la \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_split-csv.la \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_check-dcif.la \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_plot-dcif.la \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_check-syntax.la \
	$(DESTDIR)$(libdir)/yoga_SubCommand_experimental_fix-orphan.la \
	$(DESTDIR)$(libdir)/yoga_SubCommand_experimental_rotate-metric.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_experimental_cube-sampling.la \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_extensions.a \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_composite-rotor.a \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_make-composite.a \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_assemble.a \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_show-syntax.a \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_verify.a \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_split-csv.a \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_check-dcif.a \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_plot-dcif.a \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_check-syntax.a \
	$(DESTDIR)$(libdir)/yoga_SubCommand_experimental_fix-orphan.a \
	$(DESTDIR)$(libdir)/yoga_SubCommand_experimental_rotate-metric.a \
	$(DESTDIR)$(libdir)/inf_SubCommand_experimental_cube-sampling.a

uninstall-hook:
	$(RM) -f \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_extensions.so \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_composite-rotor.so \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_make-composite.so \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_assemble.so \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_show-syntax.so \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_verify.so \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_split-csv.so \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_check-dcif.so \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_plot-dcif.so \
	$(DESTDIR)$(libdir)/yoga_SubCommand_core_check-syntax.so \
	$(DESTDIR)$(libdir)/yoga_SubCommand_experimental_fix-orphan.so \
	$(DESTDIR)$(libdir)/yoga_SubCommand_experimental_rotate-metric.so \
	$(DESTDIR)$(libdir)/inf_SubCommand_experimental_cube-sampling.so


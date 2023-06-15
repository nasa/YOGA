lib_LTLIBRARIES = \
	NC_PreProcessor.la \
	libNC_PreProcessor.la \
	inf_SubCommand_core_examine.la

if BUILD_PARMETIS_SUPPORT
lib_LTLIBRARIES += \
	NC_GrpPreProcessor.la \
	libNC_GrpPreProcessor.la
endif

include_HEADERS = \
	CellRedistributor.h \
	Distributor.h \
	GraphBuilder.h \
	InitialNodeSyncer.h \
	NaiveMesh.h \
	NC_PartitionerBase.h \
	NC_PreProcessor.h \
	NC_GrpPreProcessor.h \
	NC_RCBPartitioner.h \
	PProcessor.h \
	Redistributor.h \
	PartitionGrouper.h \
	PancakePreProcessorAdapterTo.h

NC_PreProcessor_la_CXXFLAGS = @mpi_include@
NC_PreProcessor_la_CXXFLAGS += @MessagePasser_include@
NC_PreProcessor_la_CXXFLAGS += @parfait_include@
NC_PreProcessor_la_CXXFLAGS += @tracer_include@
NC_PreProcessor_la_CXXFLAGS += @t_infinity_include@
NC_PreProcessor_la_CXXFLAGS += @plugin_parfait_include@
NC_PreProcessor_la_CXXFLAGS += -I$(srcdir)

NC_PreProcessor_la_SOURCES = \
	CellRedistributor.cpp \
	GraphBuilder.cpp \
	InitialNodeSyncer.cpp \
	NaiveMesh.cpp \
	NC_PreProcessor.cpp \
	NC_PartitionerBase.cpp \
	Redistributor.cpp

NC_PreProcessor_la_LDFLAGS =  -module -no-undefined -avoid-version

NC_PreProcessor_la_LIBADD = \
	@parfait_path@/parfait/libparfait.la \
	@plugin_parfait_path@/SerialPreProcessor/libS_PreProcessor.la \
	@plugin_parfait_path@/shared/libplugin_parfait_core.la \
	@t_infinity_path@/src/t-infinity/libinfinity.la \
	@tracer_path@/include/libtracer.la

libNC_PreProcessor_la_CXXFLAGS = $(NC_PreProcessor_la_CXXFLAGS)
libNC_PreProcessor_la_SOURCES = $(NC_PreProcessor_la_SOURCES)
libNC_PreProcessor_la_LIBADD = $(NC_PreProcessor_la_LIBADD)

inf_SubCommand_core_examine_la_CXXFLAGS = @mpi_include@
inf_SubCommand_core_examine_la_CXXFLAGS += @MessagePasser_include@
inf_SubCommand_core_examine_la_CXXFLAGS += @parfait_include@
inf_SubCommand_core_examine_la_CXXFLAGS += @tracer_include@
inf_SubCommand_core_examine_la_CXXFLAGS += @t_infinity_include@
inf_SubCommand_core_examine_la_CXXFLAGS += @plugin_parfait_include@
inf_SubCommand_core_examine_la_CXXFLAGS += -I$(srcdir)
inf_SubCommand_core_examine_la_SOURCES = ExamineCommand.cpp
inf_SubCommand_core_examine_la_LDFLAGS = -module -avoid-version
inf_SubCommand_core_examine_la_LIBADD = \
	@parfait_path@/parfait/libparfait.la \
	@plugin_parfait_path@/shared/libplugin_parfait_core.la \
	@t_infinity_path@/src/t-infinity/libinfinity.la \
	@tracer_path@/include/libtracer.la

default_ldadd = \
	NC_PreProcessor.la \
	libNC_PreProcessor.la

NC_GrpPreProcessor_la_CXXFLAGS = @mpi_include@
NC_GrpPreProcessor_la_CXXFLAGS += @MessagePasser_include@
NC_GrpPreProcessor_la_CXXFLAGS += @parfait_include@
NC_GrpPreProcessor_la_CXXFLAGS += @tracer_include@
NC_GrpPreProcessor_la_CXXFLAGS += @t_infinity_include@
NC_GrpPreProcessor_la_CXXFLAGS += @plugin_parfait_include@
NC_GrpPreProcessor_la_CXXFLAGS += @parmetis_include@
NC_GrpPreProcessor_la_CXXFLAGS += -I$(srcdir)

NC_GrpPreProcessor_la_SOURCES = \
	CellRedistributor.cpp \
	GraphBuilder.cpp \
	InitialNodeSyncer.cpp \
	NaiveMesh.cpp \
	NC_GrpPreProcessor.cpp \
	NC_PartitionerBase.cpp \
	PartitionGrouper.cpp \
	Redistributor.cpp

NC_GrpPreProcessor_la_LDFLAGS =  -module -no-undefined -avoid-version

NC_GrpPreProcessor_la_LIBADD = \
	@parfait_path@/parfait/libparfait.la \
	@plugin_parfait_path@/SerialPreProcessor/libS_PreProcessor.la \
	@plugin_parfait_path@/shared/libplugin_parfait_core.la \
	@t_infinity_path@/src/t-infinity/libinfinity.la \
	@tracer_path@/include/libtracer.la \
	@parmetis_ldadd@

libNC_GrpPreProcessor_la_CXXFLAGS = $(NC_GrpPreProcessor_la_CXXFLAGS)
libNC_GrpPreProcessor_la_SOURCES = $(NC_GrpPreProcessor_la_SOURCES)
libNC_GrpPreProcessor_la_LIBADD = $(NC_GrpPreProcessor_la_LIBADD)

if BUILD_PARMETIS_SUPPORT
default_ldadd += \
	NC_GrpPreProcessor.la \
	libNC_GrpPreProcessor.la
endif

install-exec-hook:
	$(RM) -f \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_examine.la \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_examine.a

uninstall-hook:
	$(RM) -f \
	$(DESTDIR)$(libdir)/inf_SubCommand_core_examine.so


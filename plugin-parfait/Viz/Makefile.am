lib_LTLIBRARIES = \
ParfaitViz.la \
libParfaitViz.la

include_HEADERS = \
ParfaitViz.h \
ParallelTecplotWriter.h \
ParallelVTKWriter.h \
ParallelDataFrameWriter.h

ParfaitViz_la_CXXFLAGS = @mpi_include@
ParfaitViz_la_CXXFLAGS += @MessagePasser_include@
ParfaitViz_la_CXXFLAGS += @parfait_include@
ParfaitViz_la_CXXFLAGS += @tracer_include@
ParfaitViz_la_CXXFLAGS += @t_infinity_include@
ParfaitViz_la_CXXFLAGS += @plugin_parfait_include@
ParfaitViz_la_CXXFLAGS += -I$(srcdir)

ParfaitViz_la_SOURCES = \
ParfaitViz.cpp

ParfaitViz_la_LDFLAGS =  -module -no-undefined -avoid-version

ParfaitViz_la_LIBADD = \
        @parfait_path@/parfait/libparfait.la \
        @plugin_parfait_path@/shared/libplugin_parfait_core.la \
        @t_infinity_path@/src/t-infinity/libinfinity.la \
        @tracer_path@/include/libtracer.la

if BUILD_MPI
ParfaitViz_la_LIBADD += $(MPICXX_LIBS) $(MPICC_LIBS)
endif

libParfaitViz_la_CXXFLAGS = $(ParfaitViz_la_CXXFLAGS)
libParfaitViz_la_SOURCES = $(ParfaitViz_la_SOURCES)
libParfaitViz_la_LIBADD = $(ParfaitViz_la_LIBADD)

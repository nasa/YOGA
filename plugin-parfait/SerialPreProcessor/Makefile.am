lib_LTLIBRARIES = libS_PreProcessor.la

include_HEADERS = \
SerialPreProcessor.h

libS_PreProcessor_la_CXXFLAGS = @mpi_include@
libS_PreProcessor_la_CXXFLAGS += @MessagePasser_include@
libS_PreProcessor_la_CXXFLAGS += @parfait_include@
libS_PreProcessor_la_CXXFLAGS += @tracer_include@
libS_PreProcessor_la_CXXFLAGS += @t_infinity_include@

libS_PreProcessor_la_CXXFLAGS += -I$(srcdir)

libS_PreProcessor_la_SOURCES = \
SerialPreProcessor.cpp

libS_PreProcessor_la_LIBADD = \
        @plugin_parfait_path@/shared/libplugin_parfait_core.la \
        @t_infinity_path@/src/t-infinity/libinfinity.la \
        @tracer_path@/include/libtracer.la

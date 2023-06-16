lib_LTLIBRARIES = libplugin_parfait_core.la

include_HEADERS = \
CGNSReader.h \
ElementCounter.h \
EnsightReader.h \
GlobalCellIdBuilder.h \
GmshReader.h \
JsonSettings.h \
MockReaders.h \
ParmetisWrapper.h \
Reader.h \
ReaderFactory.h \
STLReader.h \
SU2IO.h \
SerialGmshExporter.h \
SerialMeshImporter.h \
SliceAcrossSections.h \
TecplotPointCloudReader.h \
TetGenReader.h \
TriReader.h \
UgridAsciiReader.h \
UgridReader.h

libplugin_parfait_core_la_CXXFLAGS = @mpi_include@
libplugin_parfait_core_la_CXXFLAGS += @MessagePasser_include@
libplugin_parfait_core_la_CXXFLAGS += @parfait_include@
libplugin_parfait_core_la_CXXFLAGS += @tracer_include@
libplugin_parfait_core_la_CXXFLAGS += @t_infinity_include@
libplugin_parfait_core_la_CXXFLAGS += -I$(srcdir)

libplugin_parfait_core_la_SOURCES = \
TetGenReader.cpp \
UgridReader.cpp \
UgridAsciiReader.cpp

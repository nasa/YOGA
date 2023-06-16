lib_LTLIBRARIES = libc_pre_processor.la

include_HEADERS = \
	c_pre_processor.h

libc_pre_processor_la_CXXFLAGS = @mpi_include@
libc_pre_processor_la_CXXFLAGS += @MessagePasser_include@
libc_pre_processor_la_CXXFLAGS += @parfait_include@
libc_pre_processor_la_CXXFLAGS += @tracer_include@
libc_pre_processor_la_CXXFLAGS += @t_infinity_include@
libc_pre_processor_la_CXXFLAGS += @t_infinity_interfaces_include@
libc_pre_processor_la_CXXFLAGS += -I$(srcdir)

libc_pre_processor_la_SOURCES = \
	c_pre_processor.cpp
if BUILD_PLUGIN
libc_pre_processor_la_LIBADD = \
	@t_infinity_path@/src/t-infinity/libinfinity.la \
	@tracer_path@/include/libtracer.la
else
libc_pre_processor_la_LIBADD = \
	../../t-infinity/libinfinity.la
endif

default_ldadd = libc_pre_processor.la


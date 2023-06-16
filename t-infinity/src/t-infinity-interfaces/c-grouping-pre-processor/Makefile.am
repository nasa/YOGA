lib_LTLIBRARIES = libc_grouping_pre_processor.la

include_HEADERS = \
    c_grouping_pre_processor.h

libc_grouping_pre_processor_la_CXXFLAGS = @mpi_include@
libc_grouping_pre_processor_la_CXXFLAGS += @MessagePasser_include@
libc_grouping_pre_processor_la_CXXFLAGS += @parfait_include@
libc_grouping_pre_processor_la_CXXFLAGS += @tracer_include@
libc_grouping_pre_processor_la_CXXFLAGS += @t_infinity_include@
libc_grouping_pre_processor_la_CXXFLAGS += @t_infinity_interfaces_include@
libc_grouping_pre_processor_la_CXXFLAGS += -I$(srcdir)

libc_grouping_pre_processor_la_SOURCES = \
    c_grouping_pre_processor.cpp

libc_grouping_pre_processor_la_LIBADD = \
	@t_infinity_path@/src/t-infinity/libinfinity.la \
	@tracer_path@/include/libtracer.la

default_ldadd = libc_grouping_pre_processor.la


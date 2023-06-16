lib_LTLIBRARIES = libdistance_calculator.la

bin_PROGRAMS = ParallelDistanceCalculator

include_HEADERS = \
    distance_calculator.h

libdistance_calculator_la_CXXFLAGS = @mpi_include@
libdistance_calculator_la_CXXFLAGS += @MessagePasser_include@
libdistance_calculator_la_CXXFLAGS += @parfait_include@
libdistance_calculator_la_CXXFLAGS += @tracer_include@
libdistance_calculator_la_CXXFLAGS += @t_infinity_include@
libdistance_calculator_la_CXXFLAGS += @t_infinity_interfaces_include@
libdistance_calculator_la_CXXFLAGS += -I$(srcdir)

libdistance_calculator_la_SOURCES = \
    distance_calculator.cpp

libdistance_calculator_la_LIBADD = \
	../../t-infinity/libinfinity.la \
	@mpi_ldadd@

default_ldadd = libdistance_calculator.la

ParallelDistanceCalculator_CXXFLAGS = \
	@mpi_include@ \
	@MessagePasser_include@ \
	@parfait_include@ \
	@tracer_include@ \
	@t_infinity_include@ \
	@t_infinity_interfaces_include@ \
	-I$(srcdir)
ParallelDistanceCalculator_SOURCES = DistanceCalculatorUtility.cpp
ParallelDistanceCalculator_LDADD = \
	../../t-infinity/libinfinity.la \
	@parfait_path@/parfait/libparfait.la \
	@t_infinity_path@/src/t-infinity/libinfinity.la \
	@tracer_path@/include/libtracer.la \
	@mpixx_ldadd@


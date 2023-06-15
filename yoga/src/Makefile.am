lib_LTLIBRARIES = libyoga.la libYogaPlugin.la libPartitionFileMeshLoader.la

include_HEADERS = \
AdtDonorFinder.h \
AlternateMapBuilder.h \
AssemblyViaExchange.h \
AssemblyViaZMQPostMan.h \
AsyncWorker.h \
BoundaryConditionParser.h \
BoundaryConditions.h \
C_InterfaceHelpers.h \
CartBlockFloodFill.h \
CartBlockGenerator.h \
CartesianLoadBalancer.h \
CellContainmentWrapper.h \
ChunkedPointGatherer.h \
ColorSyncer.h \
ComplexDifferentiator.h \
ComponentGridIdentifier.h \
Connectivity.h \
DcifChecker.h\
DcifDistributor.h \
DcifReader.h \
DcifWriter.h \
DensityEstimator.h \
Diagnostics.h \
DifferentiableDonorElement.h \
DistanceCalculator.h \
DistanceFieldAdapter.h \
DistributedLoadBalancer.h \
DomainConnectivityInfo.h \
DonorCloud.h \
DonorCollector.h \
DonorDistributor.h \
DonorPackager.h \
DonorWeightExchanger.h \
DruyorTypeAssignment.h \
ExchangeBasedAssembly.h \
FUN3DAdjointData.h \
FUN3DComplexChecker.h \
FloodFill.h \
FragmentBalancer.h \
GhostSyncPatternBuilder.h \
GlobalIdShifter.h \
GlobalIdTranslator.h \
GlobalToLocal.h \
GraphColoring.h \
GridFetcher.h \
GridRequestFulfiller.h \
HoleCutStatPrinter.h \
HoleCuttingTools.h \
InspectorPrinter.h \
InterpolationTools.h \
InterpolationTools.hpp \
Interpolator.h \
InverseReceptor.h \
IsotropicSpacingTree.h \
LagrangeElement.h \
LinearTestFunction.h \
LoadBalancer.h \
MeshDensityEstimator.h \
MeshInterfaceAdapter.h \
MeshSystemInfo.h \
MessageTracker.h \
MessageTypes.h \
MovingBodyParser.h \
NanoFlannDistanceCalculator.h \
OverDecomposer.h \
OverlapDetector.h \
OverlapMask.h \
OversetData.h \
ParallelColorCombinator.h \
ParallelSurface.h \
PartVectorIO.h \
PartitionInfo.h \
PartitionViz.h \
PortMapper.h \
QueryPoint.h \
RankTranslator.h \
Receptor.h \
ReceptorUpdate.h \
ReceptorUpdate.hpp \
RootPrinter.h \
RotorInputParser.h \
RotorPlacer.h \
ScalableHoleMap.h \
Sleep.h \
SuggarDciReader.h \
SymmetryFinder.h \
SymmetryPlane.h \
TransferNode.h \
VoxelDonorFinder.h \
VoxelFragment.h \
VoxelServer.h \
WeightBasedInterpolator.h \
WorkVoxel.h \
WorkVoxelBuilder.h \
YogaConfiguration.h \
YogaInstance.h \
YogaMesh.h \
YogaPlugin.h \
YogaStatuses.h \
YogaToTInfinityAdapter.h \
YogaTypeSupport.h \
ZMQMessager.h \
ZMQPostMan.h \
ZMQServer.hpp \
ZMQServerNameGenerator.h \
yoga_c_interface.h

AM_CXXFLAGS = \
	@mpi_include@ \
	@MessagePasser_include@ \
	@parfait_include@ \
	@tracer_include@ \
	@nanoflann_include@ \
	@t_infinity_include@ \
	@ddata_include@ \
	-I$(srcdir)

libyoga_la_SOURCES = \
YogaConfiguration.cpp \
AdtDonorFinder.cpp \
AssemblyViaExchange.cpp \
CartesianLoadBalancer.cpp \
Connectivity.cpp \
DcifChecker.cpp \
DcifDistributor.cpp \
DcifReader.cpp \
DcifWriter.cpp \
DistanceFieldAdapter.cpp \
DonorCollector.cpp \
DruyorTypeAssignment.cpp \
FragmentBalancer.cpp \
GhostSyncPatternBuilder.cpp \
GridFetcher.cpp \
HoleCuttingTools.cpp \
HoleCutStatPrinter.cpp \
MeshDensityEstimator.cpp \
MeshInterfaceAdapter.cpp \
MeshSystemInfo.cpp \
OverDecomposer.cpp \
OverlapMask.cpp \
ParallelSurface.cpp \
PartitionInfo.cpp \
ScalableHoleMap.cpp \
SuggarDciReader.cpp \
SymmetryFinder.cpp \
VoxelDonorFinder.cpp \
VoxelServer.cpp \
WorkVoxel.cpp \
WorkVoxelBuilder.cpp \
yoga_c_interface.cpp \
YogaMesh.cpp \
YogaToTInfinityAdapter.cpp \
InterpolationTools.cpp \
InterpolationToolsDdataTet.cpp \
InterpolationToolsDdataPrism.cpp \
InterpolationToolsDdataPyramid.cpp \
InterpolationToolsDdataHex.cpp

libYogaPlugin_la_SOURCES = \
	YogaPlugin.h \
	YogaPlugin.cpp
libYogaPlugin_la_LIBADD = \
	libyoga.la

libPartitionFileMeshLoader_la_SOURCES = \
	PartitionFileMeshLoader.h \
	PartitionFileMeshLoader.cpp
libPartitionFileMeshLoader_la_LIBADD = \
	libyoga.la

default_ldadd = libyoga.la


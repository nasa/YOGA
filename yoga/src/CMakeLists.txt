set(YOGA_HEADERS
        AdtDonorFinder.h
        DcifChecker.h
        DcifReader.h
        DcifWriter.h
        Diagnostics.h
        InterpolationTools.h
        InterpolationTools.hpp
        BoundaryConditions.h
        CartBlockGenerator.h
        DifferentiableDonorElement.h
        GlobalIdShifter.h
        LoadBalancer.h
        SymmetryFinder.h
        YogaMesh.h MeshDensityEstimator.h
        MeshSystemInfo.h
        SymmetryPlane.h
        CartesianLoadBalancer.h
        IsotropicSpacingTree.h
        TransferNode.h
        MessageTypes.h
        OverlapMask.h
        OversetData.h
        MeshInterfaceAdapter.h
        ParallelSurface.h
        VoxelDonorFinder.h
        PartitionInfo.h
        VoxelFragment.h
        PartitionInfo.cpp
        VoxelServer.h
        WorkVoxel.h
        DistanceFieldAdapter.h
        WorkVoxelBuilder.h
        DonorCollector.h
        YogaInstance.h
        DonorDistributor.h
        PortMapper.h
        ZMQMessager.h
        CartBlockFloodFill.h
        ../../parfait/parfait/RewindLeftHandedElements.h
        ZMQServer.hpp
        Receptor.h
        ZMQServerNameGenerator.h
        ReceptorUpdate.h
        GlobalToLocal.h
        yoga_c_interface.h
        InspectorPrinter.h
        GridFetcher.h
        ScalableHoleMap.h
        YogaPlugin.h
        RankTranslator.h
        GlobalIdTranslator.h
        OverDecomposer.h
        DonorPackager.h
        GhostSyncPatternBuilder.h
        Connectivity.h
        YogaStatuses.h
        RotorInputParser.h
        PartitionViz.h
        DruyorTypeAssignment.h
        RootPrinter.h
        OversetData.h
        YogaInstance.h
        ReceptorUpdate.h
        ParallelSurface.h
        YogaMesh.h
        YogaToTInfinityAdapter.h
        YogaTypeSupport.h
        )

set(SRC
        AdtDonorFinder.cpp
        AssemblyViaExchange.cpp
        CartesianLoadBalancer.cpp
        YogaConfiguration.cpp
        Connectivity.cpp
        DistanceFieldAdapter.cpp
        DonorCollector.cpp
        DcifChecker.cpp
        DcifDistributor.cpp
        SuggarDciReader.cpp
        DcifReader.cpp
        DcifWriter.cpp
        DruyorTypeAssignment.cpp
        FragmentBalancer.cpp
        GhostSyncPatternBuilder.cpp
        GridFetcher.cpp
        HoleCuttingTools.cpp
        HoleCutStatPrinter.cpp
        MeshDensityEstimator.cpp
        MeshInterfaceAdapter.cpp
        MeshSystemInfo.cpp
        OverDecomposer.cpp
        OverlapMask.cpp
        ParallelSurface.cpp
        ReceptorUpdate.hpp
        ScalableHoleMap.cpp
        SymmetryFinder.cpp
        VoxelDonorFinder.cpp
        VoxelServer.cpp
        WorkVoxel.cpp
        WorkVoxelBuilder.cpp
        yoga_c_interface.cpp
        YogaMesh.cpp
        InterpolationTools.cpp
        InterpolationToolsDdataTet.cpp
        InterpolationToolsDdataPrism.cpp
        InterpolationToolsDdataPyramid.cpp
        InterpolationToolsDdataHex.cpp
        YogaToTInfinityAdapter.cpp
        )

set(ZMQ_SRC
        AssemblyViaZMQPostMan.cpp
        ZMQMessager.cpp
        ZMQPostMan.cpp
        )


if(DEFINED YOGA_LINK_ZMQ)
    add_library(yoga STATIC ${SRC} ${ZMQ_SRC} ${YOGA_HEADERS})
    target_include_directories(yoga PUBLIC
            ${ZeroMQ_INCLUDE_DIR}
            )
    target_compile_definitions(yoga PRIVATE YOGA_WITH_ZMQ=1)
else()
    add_library(yoga STATIC ${SRC} ${YOGA_HEADERS})
endif()
target_include_directories(yoga PUBLIC
        ${MPI_INCLUDE_PATH}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        )
set_property(TARGET yoga PROPERTY POSITION_INDEPENDENT_CODE ON)

if (CMAKE_CXX_COMPILER MATCHES nvcc_wrapper)
    # To avoid lots of nvcc warnings about std::complex being used in CUDA kernels
    target_compile_options(yoga PRIVATE --host-only)
endif()

target_link_libraries(yoga PUBLIC
        tracer::tracer
        Threads::Threads
        infinity::infinity
        parfait::parfait
        MessagePasser::MessagePasser
        nanoflann::nanoflann
        ddata::ddata
        )
if(DEFINED YOGA_LINK_ZMQ)
    target_link_libraries(yoga PUBLIC
        ${ZeroMQ_LIBRARY}
        )
endif()

add_library(YogaPlugin SHARED YogaPlugin.cpp YogaPlugin.h)
target_link_libraries(YogaPlugin PUBLIC yoga)
set_standard_ring_rpath(YogaPlugin)

add_library(PartitionFileMeshLoader SHARED PartitionFileMeshLoader.h PartitionFileMeshLoader.cpp)
target_link_libraries(PartitionFileMeshLoader PUBLIC
        infinity::infinity
        yoga
        )
set_standard_ring_rpath(PartitionFileMeshLoader)

install(FILES ${YOGA_HEADERS}
        DESTINATION include/yoga)

install(TARGETS yoga YogaPlugin PartitionFileMeshLoader
        EXPORT yoga
        DESTINATION lib)

install(EXPORT yoga
        NAMESPACE yoga::
        DESTINATION cmake
        )

install(FILES yogaConfig.cmake
        DESTINATION cmake)

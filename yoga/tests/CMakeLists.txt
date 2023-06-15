set(UNIT_TEST_SOURCES
        parallel_sum_bug_tests.cpp
        ParallelFloodFillTests.cpp
        barycentric_coordinatesTests.cpp
        DciFDistributorTests.cpp
        least_squares_interpolationTests.cpp
        trilinear_interpolationTests.cpp
        PartitionInfoTests.cpp
        MeshSystemInfoTests.cpp
        ScalableHoleMapTests.cpp
        OverlapMaskTests.cpp
        ChunkedDistanceCalculatorTests.cpp
        AlternateMapBuilderTests.cpp
        BoundaryConditionParserTests.cpp
        GraphColoringTests.cpp
        EdgeSplitterTests.cpp
        DonorWeightExchangerTests.cpp
        TransferNodeTests.cpp
        RotorBladePlacerTests.cpp
        LagrangeBasisTests.cpp
        Fun3dRotorParserTests.cpp
        GlobalIdShifterTests.cpp
        QueryPointChunkerTests.cpp
        InverseDistanceWeightCalculator.cpp
        InterpolationToolsTests.cpp
        DonorPackagerTests.cpp
        AdtDonorFinderTests.cpp
        symmetry_planeTests.cpp
        MovingBodyInputParserTests.cpp
        NanoFlannTests.cpp
        PartitionExporterTests.cpp
        DonorWeightsTests.cpp
        ImprovedTypeAssignmentTests.cpp
        SpacingTreeTests.cpp
        StatusTransitionTests.cpp
        WorkVoxelTests.cpp
        Fun3DComplexSupportTests.cpp
        FloodFillTests.cpp
        VoxelHoleCutterTests.cpp
        #MeshDensityEstimatorTests.cpp
        ReceptorStreamingTests.cpp
        DistributedLoadBalancerTests.cpp
        #DensityTreeTests.cpp
        yoga_c_interface_tests.cpp
        MeshInterfaceAdapterFunctionTests.cpp
        GlobalIdTranslatorTests.cpp
        ConnectivityTests.cpp
        CellTesselatorTests.cpp
        SensitivityTests.cpp
        SingleCollectiveLoadBalancerTests.cpp
        WorkUnitTreeTests.cpp
        OverDecomposerTests.cpp
        YogaConfigParserTests.cpp
        ../src/WorkVoxel.cpp
         DiagonalTetsMockMesh.cpp
        )

if(DEFINED YOGA_WITH_ZMQ)
    list(APPEND UNIT_TEST_SOURCES
        ZMQPostManTests.cpp
        ../src/ZMQMessager.cpp
        DonorCollectorTests.cpp
        )
endif()

add_catch_unit_test(YogaTests ${UNIT_TEST_SOURCES})
if (CMAKE_CXX_COMPILER MATCHES nvcc_wrapper)
    # To avoid lots of nvcc warnings about std::complex being used in CUDA kernels 
    target_compile_options(YogaTests PRIVATE --host-only)
endif()
target_link_libraries(YogaTests PUBLIC YogaPlugin)
set_standard_ring_rpath(YogaTests)

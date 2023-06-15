
set(headers
        EnsightReader.h
        GlobalCellIdBuilder.h
        GmshReader.h
        JsonSettings.h
        ParmetisWrapper.h
        Reader.h
        ReaderFactory.h
        SerialGmshExporter.h
        SerialMeshImporter.h
        SliceAcrossSections.h
        SU2IO.h
        TecplotPointCloudReader.h
        TetGenReader.h
        TriReader.h
        UgridReader.h
        UgridAsciiReader.h
        )

set(sources
        TetGenReader.cpp
        UgridReader.cpp
        UgridAsciiReader.cpp
        )

if (CGNS_FOUND)
    set(headers ${headers} CGNSReader.h)
    set(headers ${headers} ElementCounter.h)
    set(sources ${sources} CGNSReader.cpp)
    set(sources ${sources} ElementCounter.cpp)
    add_subdirectory(CGNSReader/test)
endif ()

add_library(plugin_parfait_core ${headers} ${sources})
add_library(plugin-parfait::core ALIAS plugin_parfait_core)
target_link_libraries(plugin_parfait_core PUBLIC
        MessagePasser::MessagePasser
        infinity::infinity)
target_include_directories(plugin_parfait_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

if (CGNS_FOUND)
    target_compile_definitions(plugin_parfait_core PUBLIC PAR_HAVE_CGNS=1)
    target_link_libraries(plugin_parfait_core PUBLIC CGNS::CGNS)
endif ()

install(FILES ${headers}
        DESTINATION include/plugin-parfait/shared)

install(TARGETS plugin_parfait_core
        EXPORT plugin_parfait_core
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)

install(EXPORT plugin_parfait_core
        DESTINATION cmake)

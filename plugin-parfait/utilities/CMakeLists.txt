
add_executable(InternalFaceRemoverTool GmshInternalFaceRemover.cpp)
target_include_directories(InternalFaceRemoverTool PUBLIC
        ../shared
        )
target_link_libraries(InternalFaceRemoverTool PUBLIC
        infinity::infinity
        )


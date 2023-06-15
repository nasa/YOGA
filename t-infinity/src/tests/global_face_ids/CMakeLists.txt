
set(EXECUTABLE_NAME GlobalFaceIds)

add_executable(${EXECUTABLE_NAME} GlobalFaceIds.cpp)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE infinity::infinity)

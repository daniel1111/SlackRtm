# Set SOURCES variable
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Set HEADERS variable
file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# Set PUBLIC_HEADERS variable
file(GLOB PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
list(APPEND HEADERS ${PUBLIC_HEADERS})

include(${CMAKE_SOURCE_DIR}/cmake/LibraryConfig.cmake)

target_link_libraries(${LIBRARY_NAME} -lpthread)
target_link_libraries(${LIBRARY_NAME} -ljson-c)
target_link_libraries(${LIBRARY_NAME} -lwebsockets)
target_link_libraries(${LIBRARY_NAME} -lcurl)

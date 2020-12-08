cmake_minimum_required(VERSION 3.5)
project(aicup2020)

file (GLOB HEADERS "sources/strategy/*.hpp" "sources/model/*.hpp")
file (GLOB SOURCES "sources/strategy/*.cpp" "sources/model/*.cpp")
SET (MY_SOURCES 
    "sources/main.cpp"
    "sources/brain/brain.cpp"
    "sources/brain/brain.h"
    "sources/brain/building.cpp"
    "sources/brain/building.h"
    "sources/brain/fighting.cpp"
    "sources/brain/fighting.h"
    "sources/brain/state.cpp"
    "sources/brain/state.h"
    "sources/brain/utils-inl.h"
)
add_executable(aicup2020 ${HEADERS} ${SOURCES} ${MY_SOURCES})
target_include_directories(aicup2020 PRIVATE sources)

if (DEFINED LOCAL_BUILD)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
    endforeach()

    target_compile_definitions(aicup2020 PRIVATE -DWIN32)
    target_compile_options( aicup2020 PRIVATE -W3 -WX)
    target_link_libraries(aicup2020 Ws2_32.lib)

    add_custom_target(create_zip
        COMMAND
        ${CMAKE_COMMAND} -E tar "cfv" "aicup2020_sources.zip" --format=zip
           "${CMAKE_CURRENT_SOURCE_DIR}/sources"
           "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
        WORKING_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    add_dependencies(aicup2020 create_zip)
endif()

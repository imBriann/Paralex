cmake_minimum_required(VERSION 3.16)
project(Paralex VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==============================================================================
# Find MS-MPI
# ==============================================================================
find_package(MPI REQUIRED)

if(MPI_CXX_FOUND)
    message(STATUS "MPI found: ${MPI_CXX_LIBRARIES}")
    message(STATUS "MPI include: ${MPI_CXX_INCLUDE_DIRS}")
    message(STATUS "MPI compile flags: ${MPI_CXX_COMPILE_FLAGS}")
    message(STATUS "MPI link flags: ${MPI_CXX_LINK_FLAGS}")
else()
    message(FATAL_ERROR "MPI not found. Install MS-MPI Runtime and SDK.")
endif()

# ==============================================================================
# Source Files
# ==============================================================================
set(PARALEX_SOURCES
    src/utils/logger.cpp
    src/utils/text_processor.cpp
    src/utils/json_output.cpp
    src/core/document.cpp
    src/core/pipeline.cpp
    src/mapreduce/mapper.cpp
    src/mapreduce/reducer.cpp
    src/mapreduce/engine.cpp
    src/indexing/inverted_index.cpp
    src/indexing/tfidf.cpp
    src/search/search_engine.cpp
    src/similarity/cosine.cpp
    src/analytics/analyzer.cpp
    src/database/sqlite_manager.cpp
    src/database/sqlite3.c
    src/main.cpp
)

# ==============================================================================
# Executable
# ==============================================================================
add_executable(paralex_engine ${PARALEX_SOURCES})

target_include_directories(paralex_engine PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${MPI_CXX_INCLUDE_DIRS}
)

target_link_libraries(paralex_engine PRIVATE
    ${MPI_CXX_LIBRARIES}
)

if(MPI_CXX_COMPILE_FLAGS)
    target_compile_options(paralex_engine PRIVATE ${MPI_CXX_COMPILE_FLAGS})
endif()

if(MPI_CXX_LINK_FLAGS)
    set_target_properties(paralex_engine PROPERTIES LINK_FLAGS "${MPI_CXX_LINK_FLAGS}")
endif()

# ==============================================================================
# Compiler Options
# ==============================================================================
if(MSVC)
    target_compile_options(paralex_engine PRIVATE /W4 /O2 /utf-8)
    target_compile_definitions(paralex_engine PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
    target_compile_options(paralex_engine PRIVATE -Wall -Wextra -O2)
endif()

# ==============================================================================
# Output Directory
# ==============================================================================
set_target_properties(paralex_engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin"
)

message(STATUS "Paralex build configured successfully")
message(STATUS "Output: ${CMAKE_SOURCE_DIR}/bin/paralex_engine")

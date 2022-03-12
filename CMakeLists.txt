cmake_minimum_required(VERSION 3.17.0)

# Define the project
project(structured_concurrency_example LANGUAGES CXX)

if (EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)
else ()
    message(ERROR "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first")
endif ()

# Set the version of the project
project(structured_concurrency_example VERSION "${CONAN_PACKAGE_VERSION}")

# Print CMake configuration
message(STATUS "System           : ${CMAKE_SYSTEM}")
message(STATUS "System name      : ${CMAKE_SYSTEM_NAME}")
message(STATUS "System ver       : ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Conan compiler   : ${CONAN_SETTINGS_COMPILER} ${CONAN_SETTINGS_COMPILER_VERSION}")
message(STATUS "Conan libcxx     : ${CONAN_SETTINGS_COMPILER_LIBCXX}")
message(STATUS)

set(sourceFiles
    src/http_server/create_response.cpp
    src/http_server/request_parser.cpp
    src/http_server/to_buffers.cpp

    src/io/detail/poll_io_loop.cpp
    src/io/listening_socket.cpp
    src/io/connection.cpp

    src/main.cpp
    )

add_executable(image_server ${sourceFiles})


# Set the version and current build date
set(image_server_VERSION "${PROJECT_VERSION}")
set(image_server_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
string(TIMESTAMP image_server_BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP image_server_BUILD_YEAR "%Y")

message(STATUS "Library ver      : ${image_server_VERSION}")
message(STATUS "Build date       : ${image_server_BUILD_DATE}")
message(STATUS "Build year       : ${image_server_BUILD_YEAR}")
message(STATUS)

# The include directories for image_server
target_include_directories(image_server PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}/src/
                           ${CMAKE_CURRENT_SOURCE_DIR}/external/p2300/include/
                           ${CMAKE_CURRENT_SOURCE_DIR}/external/p2300/examples/
                           )

# Profiling
if (TARGET CONAN_PKG::tracy-interface)
    target_link_libraries(image_server PRIVATE CONAN_PKG::tracy-interface)
    target_compile_definitions(image_server PRIVATE PROFILING_ENABLED=1)
endif ()

# Ensure that we link with the threading library
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
target_link_libraries(image_server PRIVATE Threads::Threads)

# Use C++20 standard
target_compile_features(image_server PRIVATE cxx_std_20)

# Turn all warnings
target_compile_options(image_server PRIVATE
                       $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
                       -Wall>
                       $<$<CXX_COMPILER_ID:MSVC>:
                       /W4>)
# template backtrace limit
target_compile_options(image_server PRIVATE
                       $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
                       -ftemplate-backtrace-limit=0>
                       )
# Proper use of libc++
if (CONAN_SETTINGS_COMPILER_LIBCXX STREQUAL "libc++")
    target_compile_options(image_server PRIVATE -stdlib=libc++)
    target_link_options(image_server PRIVATE -lc++)
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
endif ()


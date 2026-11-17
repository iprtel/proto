cmake_minimum_required(VERSION 3.28)

project(Proto LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        95ba44311c8c05dcea093d218793afd3f19e1c6e
  SYSTEM)
FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse
  GIT_TAG        3eda91b2e1ce7d569f84ba295507c4cd8fd96910
  SYSTEM)
FetchContent_MakeAvailable(fmt argparse)

if(PROJECT_IS_TOP_LEVEL)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
      SYSTEM)
    FetchContent_MakeAvailable(googletest)

    # Warnings for this project's own targets only. Dependencies are added
    # above, so they are not affected, and projects that embed this one keep
    # their own settings.
    if(MSVC)
        add_compile_options(/W4)
    else()
        add_compile_options(-Wall -Wextra -Wpedantic)
    endif()
endif()

# The header-only library. Linking iprtl::proto adds the include path, {fmt} and C++20.
add_library(iprtl_proto INTERFACE)
add_library(iprtl::proto ALIAS iprtl_proto)
target_include_directories(iprtl_proto INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(iprtl_proto INTERFACE fmt::fmt)
target_compile_features(iprtl_proto INTERFACE cxx_std_20)

# protopiler_add_library(<name> <file.proto>...)
#
# Runs protopiler on each .proto file (relative to the current source directory)
# and creates an INTERFACE library <name>. Linking <name> provides the generated
# <file>.proto.h headers and iprtl::proto, and makes sure they are generated first.
function(protopiler_add_library name)
    set(out_dir ${CMAKE_CURRENT_BINARY_DIR}/${name})
    set(headers)
    foreach(proto IN LISTS ARGN)
        cmake_path(ABSOLUTE_PATH proto BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        cmake_path(GET proto FILENAME filename)
        set(header ${out_dir}/${filename}.h)
        add_custom_command(
            OUTPUT ${header}
            COMMAND protopiler --out-dir ${out_dir} ${proto}
            DEPENDS protopiler ${proto}
            COMMENT "Generating ${filename}.h")
        list(APPEND headers ${header})
    endforeach()
    add_custom_target(${name}-generate DEPENDS ${headers})
    add_library(${name} INTERFACE)
    add_dependencies(${name} ${name}-generate)
    target_include_directories(${name} INTERFACE ${out_dir})
    target_link_libraries(${name} INTERFACE iprtl::proto)
endfunction()

add_subdirectory(src/iprtl/proto/compiler)
add_subdirectory(src/iprtl/proto/tools)

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(src/examples/datetime)

    enable_testing()
    add_subdirectory(tests)
endif()

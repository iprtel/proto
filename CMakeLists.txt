 cmake_minimum_required(VERSION 3.28)

project(Proto LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

include(FetchContent)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        95ba44311c8c05dcea093d218793afd3f19e1c6e)
FetchContent_MakeAvailable(fmt)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)
FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse
  GIT_TAG        3eda91b2e1ce7d569f84ba295507c4cd8fd96910)
FetchContent_MakeAvailable(argparse)

include_directories("src")
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
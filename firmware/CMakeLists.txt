cmake_minimum_required(VERSION 3.16.0)
include(FetchContent)
FetchContent_Declare(
  espidflib
  GIT_REPOSITORY https://github.com/UncleRus/esp-idf-lib.git
  GIT_TAG        edf9ba17056dfc376eeff31bbf105cdff407b1a6
)
FetchContent_MakeAvailable(espidflib)
list(APPEND EXTRA_COMPONENT_DIRS ${espidflib_SOURCE_DIR}/components)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(chronovfd)

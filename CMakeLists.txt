cmake_minimum_required(VERSION 3.0)

# Include `vrm_cmake`.
list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/../vrm_cmake/cmake/"
    "${CMAKE_SOURCE_DIR}/extlibs/vrm_cmake/cmake/")

include(vrm_cmake)

# Basic project setup.
vrm_cmake_init_project(WEBVittorioRomeo2)
vrm_cmake_find_extlib(vrm_pp)
vrm_cmake_find_extlib(SSVUtils)
find_library(LIB_MARKDOWN markdown)
find_path(INC_MARKDOWN mkdio.h)

vrm_cmake_add_common_compiler_flags()

include_directories(${INC_MARKDOWN})
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${LIB_MARKDOWN})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/_RELEASE/)

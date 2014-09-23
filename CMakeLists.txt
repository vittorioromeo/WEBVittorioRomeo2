cmake_minimum_required(VERSION 2.8)
project(WEBVittorioRomeo2)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/extlibs/SSVCMake/cmake/modules/;${CMAKE_MODULE_PATH}")
include(SSVCMake)

SSVCMake_setDefaults()
SSVCMake_findExtlib(SSVUtils)
SSVCMake_findExtlib(SSVUtilsJson)
find_library(LIB_MARKDOWN markdown)
find_path(INC_MARKDOWN mkdio.h)

include_directories(${INC_MARKDOWN})
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${LIB_MARKDOWN})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/_RELEASE/)

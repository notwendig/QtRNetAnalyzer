cmake_minimum_required(VERSION 3.21)
project(QtRNetAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

file(GLOB SRC_FILES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.h
)

add_executable(QtRNetAnalyzer ${SRC_FILES})

target_include_directories(QtRNetAnalyzer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

set(CONTROLCAN_LIB
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/x86/64-linux/libcontrolcan.so
)

if(EXISTS ${CONTROLCAN_LIB})
    message(STATUS "Using ControlCAN library")

    add_library(controlcan SHARED IMPORTED GLOBAL)
    set_target_properties(controlcan PROPERTIES
        IMPORTED_LOCATION ${CONTROLCAN_LIB}
    )

    target_link_libraries(QtRNetAnalyzer PRIVATE controlcan)

    set_target_properties(QtRNetAnalyzer PROPERTIES
        BUILD_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/x86/64-linux"
        INSTALL_RPATH "$ORIGIN/../third_party/x86/64-linux"
    )
else()
    message(WARNING "ControlCAN library not found -> build without hardware support")
endif()

target_link_libraries(QtRNetAnalyzer PRIVATE
    Qt6::Core
    Qt6::Widgets
)

cmake_minimum_required(VERSION 3.19)

project(QtRNetAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()

qt_add_executable(QtRNetAnalyzer
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/canframe.h
    src/canlogger.cpp
    src/canlogger.h
    src/controlcandeviceworker.cpp
    src/controlcandeviceworker.h
    src/liveframedelegate.cpp
    src/liveframedelegate.h
    src/liveframemodel.cpp
    src/liveframemodel.h
    src/rnetframe.cpp
    src/rnetframe.h
    src/rnetframedelegate.cpp
    src/rnetframedelegate.h
    src/rnetframemodel.cpp
    src/rnetframemodel.h
    src/rnetframesimulator.cpp
    src/rnetframesimulator.h
    src/rnetsource.h
    src/signalsample.h
    src/signalhistorymodel.cpp
    src/signalhistorymodel.h
    src/signalplotwidget.cpp
    src/signalplotwidget.h
    src/signalviewwindow.cpp
    src/signalviewwindow.h
)

target_include_directories(QtRNetAnalyzer PRIVATE src)
target_link_libraries(QtRNetAnalyzer PRIVATE Qt6::Core Qt6::Widgets)

# Optional proprietary ControlCAN SDK support.
# Full hardware build is enabled only when BOTH the vendor header and library
# are present. If only controlcan.h exists, the app still builds in
# simulation/candump replay mode instead of failing at link time.
set(QTRA_CONTROLCAN_INCLUDE_DIR "")
set(QTRA_CONTROLCAN_LIBRARY "")

set(QTRA_CONTROLCAN_SEARCH_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/controlcan"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ControlCAN"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
)

foreach(_dir IN LISTS QTRA_CONTROLCAN_SEARCH_DIRS)
    if(EXISTS "${_dir}/controlcan.h" AND NOT QTRA_CONTROLCAN_INCLUDE_DIR)
        set(QTRA_CONTROLCAN_INCLUDE_DIR "${_dir}")
    endif()
endforeach()

find_library(QTRA_CONTROLCAN_LIBRARY
    NAMES controlcan ControlCAN libcontrolcan libControlCAN
    PATHS ${QTRA_CONTROLCAN_SEARCH_DIRS}
    NO_DEFAULT_PATH
)

if(QTRA_CONTROLCAN_INCLUDE_DIR AND QTRA_CONTROLCAN_LIBRARY)
    message(STATUS "ControlCAN enabled: ${QTRA_CONTROLCAN_LIBRARY}")
    target_compile_definitions(QtRNetAnalyzer PRIVATE QTRA_HAS_CONTROLCAN=1)
    target_include_directories(QtRNetAnalyzer PRIVATE "${QTRA_CONTROLCAN_INCLUDE_DIR}")
    target_link_libraries(QtRNetAnalyzer PRIVATE "${QTRA_CONTROLCAN_LIBRARY}")
else()
    message(STATUS "ControlCAN disabled: vendor header and library were not both found")
    target_compile_definitions(QtRNetAnalyzer PRIVATE QTRA_HAS_CONTROLCAN=0)
endif()

install(TARGETS QtRNetAnalyzer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

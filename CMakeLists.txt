cmake_minimum_required(VERSION 3.21)

project(QtRNetAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

set(QTRNET_SOURCES
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

qt_add_executable(QtRNetAnalyzer
    ${QTRNET_SOURCES}
)

target_include_directories(QtRNetAnalyzer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(QtRNetAnalyzer
    PRIVATE
        Qt6::Core
        Qt6::Widgets
)

#
# Optional/full ControlCAN SDK handling
#
# Policy:
#   - If third_party/controlcan.h exists, build the full hardware version.
#   - For the full version the matching vendor library is required.
#   - Expected repository layout:
#
#       third_party/controlcan.h
#       third_party/x86/64-linux/libcontrolcan.so
#       third_party/x86/32-linux/libcontrolcan.so
#
set(CONTROLCAN_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/third_party/controlcan.h")

if(EXISTS "${CONTROLCAN_HEADER}")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CONTROLCAN_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/x86/64-linux")
    else()
        set(CONTROLCAN_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/x86/32-linux")
    endif()

    find_library(CONTROLCAN_LIBRARY
        NAMES controlcan libcontrolcan
        PATHS "${CONTROLCAN_LIB_DIR}"
        NO_DEFAULT_PATH
    )

    if(NOT CONTROLCAN_LIBRARY)
        message(FATAL_ERROR
            "third_party/controlcan.h exists, therefore QtRNetAnalyzer must be built "
            "as the full ControlCAN version, but no matching libcontrolcan library "
            "was found in: ${CONTROLCAN_LIB_DIR}"
        )
    endif()

    target_include_directories(QtRNetAnalyzer
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    )

    target_link_libraries(QtRNetAnalyzer
        PRIVATE
            ${CONTROLCAN_LIBRARY}
    )

    target_compile_definitions(QtRNetAnalyzer
        PRIVATE
            QTRNETANALYZER_WITH_CONTROLCAN=1
    )

    set_target_properties(QtRNetAnalyzer PROPERTIES
        BUILD_RPATH "${CONTROLCAN_LIB_DIR}"
        INSTALL_RPATH "$ORIGIN;${CONTROLCAN_LIB_DIR}"
    )

    message(STATUS "ControlCAN full hardware build enabled")
    message(STATUS "ControlCAN header: ${CONTROLCAN_HEADER}")
    message(STATUS "ControlCAN library: ${CONTROLCAN_LIBRARY}")
else()
    message(WARNING
        "third_party/controlcan.h not found. This source tree currently expects "
        "the ControlCAN SDK for hardware capture. Simulation replay still works "
        "only if the source code is built with a compatible SDK/stub setup."
    )
endif()

install(TARGETS QtRNetAnalyzer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

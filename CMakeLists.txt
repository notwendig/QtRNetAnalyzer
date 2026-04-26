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

target_include_directories(QtRNetAnalyzer PRIVATE
    src
)

# Optional proprietary ControlCAN SDK support.
#
# Rule:
#   - If third_party/controlcan.h exists, build the full ControlCAN-enabled version.
#   - Otherwise build the simulation/candump-only version.
#
# The source code uses __has_include("controlcan.h"), so adding third_party/ to
# the include path is enough to enable the full implementation.
set(QTRA_CONTROLCAN_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/third_party/controlcan.h")

if(EXISTS "${QTRA_CONTROLCAN_HEADER}")
    message(STATUS "ControlCAN SDK detected: ${QTRA_CONTROLCAN_HEADER}")
    message(STATUS "Building QtRNetAnalyzer with ControlCAN hardware support")

    target_include_directories(QtRNetAnalyzer PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
    )

    target_compile_definitions(QtRNetAnalyzer PRIVATE
        QTRA_CONTROLCAN_AVAILABLE=1
    )

    # Link vendor library if it is provided in third_party/.
    # Header-only detection still enables the full source implementation; if the
    # SDK library is required but missing, the linker will report the missing VCI
    # symbols clearly instead of silently building a fake hardware version.
    find_library(QTRA_CONTROLCAN_LIBRARY
        NAMES controlcan ControlCAN libcontrolcan libControlCAN
        PATHS "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
        NO_DEFAULT_PATH
    )

    if(QTRA_CONTROLCAN_LIBRARY)
        message(STATUS "ControlCAN library detected: ${QTRA_CONTROLCAN_LIBRARY}")
        target_link_libraries(QtRNetAnalyzer PRIVATE "${QTRA_CONTROLCAN_LIBRARY}")
    else()
        message(WARNING
            "third_party/controlcan.h exists, so the full ControlCAN source path "
            "will be compiled, but no ControlCAN library was found in third_party/. "
            "If your SDK requires a library, place libcontrolcan.so, libControlCAN.so, "
            "libcontrolcan.a, or libControlCAN.a there."
        )
    endif()
else()
    message(STATUS "ControlCAN SDK not found: building simulation/candump-only version")
    target_compile_definitions(QtRNetAnalyzer PRIVATE
        QTRA_CONTROLCAN_AVAILABLE=0
    )
endif()

target_link_libraries(QtRNetAnalyzer PRIVATE
    Qt6::Core
    Qt6::Widgets
)

install(TARGETS QtRNetAnalyzer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

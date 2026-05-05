cmake_minimum_required(VERSION 3.21)

project(QtRNetAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()

option(QTRNET_ENABLE_WAVESHARE_USBCANB "Enable Waveshare USBCAN-B / CANalyst-II lowlevel backend" ON)

set(QTRNET_WAVESHARE_TARGET "")
set(QTRNET_WAVESHARE_SUBPROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/waveshares_USBCAN_B")

if(QTRNET_ENABLE_WAVESHARE_USBCANB)
    if(EXISTS "${QTRNET_WAVESHARE_SUBPROJECT_DIR}/CMakeLists.txt")
        add_subdirectory("${QTRNET_WAVESHARE_SUBPROJECT_DIR}" EXCLUDE_FROM_ALL)

        if(TARGET QUsbCanBLowLevelStatic)
            set(QTRNET_WAVESHARE_TARGET QUsbCanBLowLevelStatic)
        elseif(TARGET qusbcanb::lowlevel_static)
            set(QTRNET_WAVESHARE_TARGET qusbcanb::lowlevel_static)
        elseif(TARGET qusbcanb::lowlevel_shared)
            set(QTRNET_WAVESHARE_TARGET qusbcanb::lowlevel_shared)
        else()
            message(FATAL_ERROR "waveshares_USBCAN_B subproject was found, but no lowlevel CMake target was exported")
        endif()

        message(STATUS "Waveshare USBCAN-B subproject enabled: ${QTRNET_WAVESHARE_SUBPROJECT_DIR}")
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
                set(ENV{PKG_CONFIG_PATH} "$ENV{HOME}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
            endif()
            pkg_check_modules(QUSBCANB_LOWLEVEL QUIET IMPORTED_TARGET qusbcanb_lowlevel)
            if(QUSBCANB_LOWLEVEL_FOUND)
                set(QTRNET_WAVESHARE_TARGET PkgConfig::QUSBCANB_LOWLEVEL)
                message(STATUS "Waveshare USBCAN-B lowlevel pkg-config dependency enabled")
            endif()
        endif()
    endif()
endif()

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

if(QTRNET_WAVESHARE_TARGET)
    target_link_libraries(QtRNetAnalyzer PRIVATE ${QTRNET_WAVESHARE_TARGET})
    target_compile_definitions(QtRNetAnalyzer PRIVATE QTRA_HAS_WAVESHARE_USBCANB=1)

    if(DEFINED ENV{HOME} AND EXISTS "$ENV{HOME}/lib")
        set_target_properties(QtRNetAnalyzer PROPERTIES
            BUILD_RPATH "$ENV{HOME}/lib"
            INSTALL_RPATH "$ORIGIN;$ENV{HOME}/lib"
        )
    endif()
else()
    target_compile_definitions(QtRNetAnalyzer PRIVATE QTRA_HAS_WAVESHARE_USBCANB=0)
    message(WARNING "Waveshare USBCAN-B lowlevel backend not found. Hardware capture is disabled; simulation replay still builds.\n"
                    "To enable hardware capture, either run:\n"
                    "  tools/add_waveshares_usbcan_submodule.sh\n"
                    "or install waveshares_USBCAN_B to ~/lib and export PKG_CONFIG_PATH=\$HOME/lib/pkgconfig")
endif()

install(TARGETS QtRNetAnalyzer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

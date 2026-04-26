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

# Optional proprietary ControlCAN SDK support. The app must still build
# without the vendor header/library; in that case the hardware worker is a
# clean stub and simulation/candump replay remains available.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/controlcan/controlcan.h")
    target_include_directories(QtRNetAnalyzer PRIVATE third_party/controlcan)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ControlCAN/controlcan.h")
    target_include_directories(QtRNetAnalyzer PRIVATE third_party/ControlCAN)
endif()

target_link_libraries(QtRNetAnalyzer PRIVATE Qt6::Core Qt6::Widgets)

install(TARGETS QtRNetAnalyzer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

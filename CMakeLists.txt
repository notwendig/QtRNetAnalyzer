cmake_minimum_required(VERSION 3.21)

project(QtRNetAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets SerialBus LinguistTools)

qt_standard_project_setup()

option(QTRNET_ENABLE_SOCKETCAN "Enable Linux SocketCAN hardware capture backend" ON)
option(QTRNET_ENABLE_DANGEROUS_TX "Enable dangerous laboratory-only CAN transmit UI" OFF)


#
# External subprojects
#
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/RNetMsgBroker/CMakeLists.txt")
  message(FATAL_ERROR
    "external/RNetMsgBroker fehlt. Bitte Submodule initialisieren:\n"
    "git submodule update --init --recursive"
  )
endif()

add_subdirectory(external/RNetMsgBroker)

set(QTRNET_SOURCES
  src/main.cpp
  src/appsettingscontroller.cpp
  src/appsettingscontroller.h
  src/mainwindow.cpp
  src/mainwindow.h
  src/canlogger.cpp
  src/canlogger.h
  src/controlcandeviceworker.cpp
  src/controlcandeviceworker.h
  src/liveframedelegate.cpp
  src/liveframedelegate.h
  src/liveframemodel.cpp
  src/liveframemodel.h
  src/rnetframedelegate.cpp
  src/rnetframedelegate.h
  src/rnetframemodel.cpp
  src/rnetframemodel.h
  src/rnetframesimulator.cpp
  src/rnetframesimulator.h
  src/rnetwheelchairsimulator.cpp
  src/rnetwheelchairsimulator.h
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

set(QTRNET_TRANSLATION_FILES
  translations/QtRNetAnalyzer_de_DE.ts
)

qt_add_translations(QtRNetAnalyzer
  TS_FILES
    ${QTRNET_TRANSLATION_FILES}
  RESOURCE_PREFIX "/i18n"
)

target_include_directories(QtRNetAnalyzer
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(QtRNetAnalyzer
  PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::SerialBus
    RNetMsgBroker
)

if(QTRNET_ENABLE_DANGEROUS_TX)
    target_compile_definitions(QtRNetAnalyzer PRIVATE QTRNET_ENABLE_DANGEROUS_TX=1)
    message(WARNING "Dangerous CAN TX is enabled. Use only on isolated lab wiring.")
else()
    target_compile_definitions(QtRNetAnalyzer PRIVATE QTRNET_ENABLE_DANGEROUS_TX=0)
endif()

if(QTRNET_ENABLE_SOCKETCAN AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_definitions(QtRNetAnalyzer PRIVATE QTRA_HAS_SOCKETCAN=1)
  message(STATUS "Linux SocketCAN backend enabled. Use waveUSBCAN_b DKMS driver to expose can0/can1.")
else()
  target_compile_definitions(QtRNetAnalyzer PRIVATE QTRA_HAS_SOCKETCAN=0)
  message(WARNING "SocketCAN backend disabled. Hardware capture is unavailable; simulation replay still builds.")
endif()

install(TARGETS QtRNetAnalyzer
  BUNDLE DESTINATION .
  RUNTIME DESTINATION bin
)


add_custom_command(TARGET QtRNetAnalyzer POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/external/RNetMsgBroker/R-Net.json
    $<TARGET_FILE_DIR:QtRNetAnalyzer>/R-Net.json
)

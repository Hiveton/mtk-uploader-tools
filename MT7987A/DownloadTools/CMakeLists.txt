cmake_minimum_required(VERSION 3.20)

project(MT7987ADownloadTools VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort)

qt_add_executable(MT7987ADownloadTools
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/BoardModel.h
    src/BoardModel.cpp
    src/BoardConfigDialog.h
    src/BoardConfigDialog.cpp
    src/DownloadCommand.h
    src/DownloadCommand.cpp
    src/DownloadProcess.h
    src/DownloadProcess.cpp
)

target_link_libraries(MT7987ADownloadTools PRIVATE Qt6::Widgets Qt6::SerialPort)

if(MSVC)
    target_compile_options(MT7987ADownloadTools PRIVATE /utf-8)
endif()

if(WIN32)
    set_target_properties(MT7987ADownloadTools PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
    set_target_properties(MT7987ADownloadTools PROPERTIES MACOSX_BUNDLE TRUE)
endif()

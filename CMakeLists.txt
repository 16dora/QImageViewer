cmake_minimum_required(VERSION 3.16)
project(QimageViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR}
    COMPONENTS
        Core
        Gui
        Widgets
)

if(QT_VERSION VERSION_LESS 6.3)
    macro(qt_standard_project_setup)
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTOUIC ON)
    endmacro()
endif()

if(QT_VERSION VERSION_LESS 6.0)
    macro(qt_add_executable name)
        if(ANDROID)
            add_library(${name} SHARED ${ARGN})
        else()
            add_executable(${ARGV})
        endif()
    endmacro()
endif()

qt_standard_project_setup()

set(PROJECT_SOURCES
    source/main.cpp
    source/QimageViewer.cpp
    source/ImgGraphicsView.cpp
    include/QimageViewer.h
    include/ImgGraphicsView.h
    uidesign/QimageViewer.ui
)

qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/uidesign
        WIN32_EXECUTABLE TRUE
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
)


cmake_minimum_required(VERSION 3.16)
project(QimageViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(NOT WIN32 OR NOT MINGW)
    message(FATAL_ERROR "This project currently supports Qwt only with Windows MinGW64.")
endif()

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR}
    REQUIRED
    COMPONENTS
        Core
        Gui
        OpenGL
        OpenGLWidgets
        Svg
        Widgets
)

set(QWT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/qwt")
set(QWT_INCLUDE_DIR "${QWT_ROOT}/include")
set(QWT_IMPORT_LIBRARY "${QWT_ROOT}/bin/mingw_win64/libqwt.a")
set(QWT_RUNTIME_LIBRARY "${QWT_ROOT}/bin/mingw_win64/qwt.dll")

foreach(QWT_REQUIRED_PATH
        "${QWT_INCLUDE_DIR}/qwt_plot.h"
        "${QWT_IMPORT_LIBRARY}"
        "${QWT_RUNTIME_LIBRARY}")
    if(NOT EXISTS "${QWT_REQUIRED_PATH}")
        message(FATAL_ERROR "Required Qwt dependency not found: ${QWT_REQUIRED_PATH}")
    endif()
endforeach()

add_library(Qwt::Qwt SHARED IMPORTED)
set_target_properties(Qwt::Qwt
    PROPERTIES
        IMPORTED_IMPLIB "${QWT_IMPORT_LIBRARY}"
        IMPORTED_LOCATION "${QWT_RUNTIME_LIBRARY}"
        INTERFACE_COMPILE_DEFINITIONS QWT_DLL
        INTERFACE_INCLUDE_DIRECTORIES "${QWT_INCLUDE_DIR}"
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
        Qwt::Qwt
        Qt::Core
        Qt::Gui
        Qt::OpenGL
        Qt::OpenGLWidgets
        Qt::Svg
        Qt::Widgets
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Qwt::Qwt>"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMENT "Copying Qwt runtime library"
)


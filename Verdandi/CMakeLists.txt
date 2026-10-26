cmake_minimum_required(VERSION 3.16)

project(Verdandi VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Qml Quick Network Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Qml Quick Network Sql)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Verdandi
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        apiclient.h apiclient.cpp
        databasemanager.h databasemanager.cpp
        resources.qrc

    )
else()
    add_executable(Verdandi ${PROJECT_SOURCES})
endif()

target_link_libraries(Verdandi PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)
target_link_libraries(Verdandi PRIVATE Qt${QT_VERSION_MAJOR}::Core)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Verdandi)
endif()
set_target_properties(Verdandi PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Verdandi
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Verdandi)
endif()

# add_custom_command(TARGET Verdandi POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:PluginExample>/plugins
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:PluginExample> $<TARGET_FILE_DIR:PluginExample>/plugins
#     COMMENT "Copying plugin to plugins directory"
#)

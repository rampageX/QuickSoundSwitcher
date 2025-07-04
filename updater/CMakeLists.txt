cmake_minimum_required(VERSION 3.16)

find_package(Qt6 REQUIRED COMPONENTS Quick Network)

qt_add_executable(QSSUpdater
    src/main.cpp
    src/updatemanager.cpp
    include/updatemanager.h
)

qt_add_qml_module(QSSUpdater
    URI Odizinne.QSSUpdater
    VERSION 1.0
    QML_FILES qml/Main.qml
)

target_include_directories(QSSUpdater PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(QSSUpdater PRIVATE
    Qt::Quick
    Qt::Network
)

# Copy to main output directory
add_custom_command(TARGET QSSUpdater POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:QSSUpdater> ${CMAKE_BINARY_DIR}/
)
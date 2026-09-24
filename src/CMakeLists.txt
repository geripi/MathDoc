# Copyright (C) 2024 Gerald Pichler
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.


#qt6_add_resources(mathdoc_SRCS resources.qrc)
#aux_source_directory(. MATHDOC_SOURCES) # The '.' means current directory
#add_executable(mathdoc main.cpp ${mathdoc_SRCS} ${MATHDOC_SOURCES})
#target_link_libraries(mathdoc Qt6::Core Qt6::Gui Qt6::Widgets)

# Add the source and header files for your executable
set(MATHDOC_SOURCES
    main.cpp
    data.cpp
    unit.cpp
    mainwindow.cpp
    mditabbeddocumentarea.cpp
    documentwidget.cpp
    pagegraphicsscene.cpp
    pagegraphicsview.cpp
    keycapobject.cpp
    pagetextitem.cpp
    pagemathitem.cpp
    pagediagramitem.cpp
    #mathleafedit.cpp
    matheditfactory.cpp
    matheditcreatenew.cpp
    mathedit.cpp
    matheditoperator.cpp
    matheditconstant.cpp
    matheditvariable.cpp
    mathvariable.cpp
    matheditexponent.cpp
    matheditfraction.cpp
    matheditfunction.cpp
    matheditlog.cpp
    matheditroot.cpp
    matheditconditionalfunc.cpp
    diagramtitle.cpp
    pageimageitem.cpp
    # Add other .h files here
    helpers.h
    floatlistvalidator.h
    data.h
    unit.h
    mainwindow.h
    mditabbeddocumentarea.h
    documentwidget.h
    pagegraphicsscene.h
    pagegraphicsview.h
    pageA4item.h
    keycapobject.h
    pagetextitem.h
    pagemathitem.h
    pagediagramitem.h
    #mathleafedit.h
    matheditfactory.h
    matheditcreatenew.h
    mathedit.h
    matheditoperator.h
    matheditconstant.h
    matheditvariable.h
    mathvariable.h
    matheditexponent.h
    matheditfraction.h
    matheditfunction.h
    matheditlog.h
    matheditroot.h
    matheditconditionalfunc.h
    diagramtitle.h
    pageimageitem.h
)

# Handle Qt resources
qt6_add_resources(MATHDOC_SOURCES resources.qrc)

# Create the executable
add_executable(mathdoc MACOSX_BUNDLE ${MATHDOC_SOURCES})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mathdoc PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Werror
    )
elseif(MSVC)
    target_compile_options(mathdoc PRIVATE
        /W4
    )
endif()

# Enable profiling for debug builds
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(mathdoc PRIVATE -pg)
endif()

# Explicitly link to all necessary Qt modules
target_link_libraries(mathdoc PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Tell the build system where to find your project's headers.
# This is the most crucial step for a semantic analyzer.
# The 'PRIVATE' keyword means only this target will use this include directory.
target_include_directories(mathdoc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

get_target_property(MATHDOC_INCLUDE_DIRS mathdoc INCLUDE_DIRECTORIES)
message(STATUS "MATHDOC_INCLUDE_DIRECTORIES: ${MATHDOC_INCLUDE_DIRS}")

get_target_property(MATHDOC_COMPILE_DEFINITIONS mathdoc COMPILE_DEFINITIONS)
message(STATUS "MATHDOC_COMPILE_DEFINITIONS: ${MATHDOC_COMPILE_DEFINITIONS}")

get_target_property(MATHDOC_COMPILE_OPTIONS mathdoc COMPILE_OPTIONS)
message(STATUS "MATHDOC_COMPILE_OPTIONS: ${MATHDOC_COMPILE_OPTIONS}")

# You may also need the flags from the linked Qt modules
get_target_property(QT_WIDGETS_INCLUDE_DIRS Qt6::Widgets INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "QT6_WIDGETS_INCLUDE_DIRECTORIES: ${QT_WIDGETS_INCLUDE_DIRS}")
get_target_property(QT_WIDGETS_COMPILE_OPTIONS Qt6::Widgets INTERFACE_COMPILE_OPTIONS)
message(STATUS "QT6_WIDGETS_COMPILE_OPTIONS: ${QT_WIDGETS_COMPILE_OPTIONS}")

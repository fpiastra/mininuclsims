#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(mininucldsims)


#----------------------------------------------------------------------------
#Uncomment the line here below if you want the debug symbols and macros defined
set(CMAKE_BUILD_TYPE Debug)

#----------------------------------------------------------------------------
# Find Geant4 package, activating all available UI and Vis drivers by default
# You can set WITH_GEANT4_UIVIS to OFF via the command line or ccmake/cmake-gui
# to build a batch mode only executable
#

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "$ENV(HOME)/software/nlohmann_json/lib/cmake")
#find_package(nlohmann_json REQUIRED)


option(WITH_GEANT4_UIVIS "Build the mininucldsims simulation code with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
  find_package(Geant4 REQUIRED ui_all vis_all)
else()
  find_package(Geant4 REQUIRED)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "$ENV{ROOTSYS}/etc/cmake")
find_package(ROOT REQUIRED)




#----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
# Setup include directory for this project
#
include(${Geant4_USE_FILE})
#include(${ROOT_USE_FILE})

#----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${Geant4_INCLUDE_DIR}
                    $ENV{ROOTSYS}/include
                    ${ROOT_INCLUDE_DIR}
                    )
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Create an object library from the common sources
#
add_library(common_objs OBJECT ${sources})

#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
add_executable(mininuclsims ${PROJECT_SOURCE_DIR}/prog/mininuclsims.cpp $<TARGET_OBJECTS:common_objs>)
target_link_libraries(mininuclsims ${Geant4_LIBRARIES} ${ROOT_LIBRARIES})

add_executable(mininuclsims-PVlist ${PROJECT_SOURCE_DIR}/prog/mininuclsims-PVlist.cpp $<TARGET_OBJECTS:common_objs>)
target_link_libraries(mininuclsims-PVlist ${Geant4_LIBRARIES} ${ROOT_LIBRARIES})

add_executable(mininuclsims-LVlist ${PROJECT_SOURCE_DIR}/prog/mininuclsims-LVlist.cpp $<TARGET_OBJECTS:common_objs>)
target_link_libraries(mininuclsims-LVlist ${Geant4_LIBRARIES} ${ROOT_LIBRARIES})

#add_executable(mininucldsims-PVscan ${PROJECT_SOURCE_DIR}/prog/mininucldsims-PVscan.cpp $<TARGET_OBJECTS:common_objs>)
#target_link_libraries(ArgonCubeOptPh-PVscan ${Geant4_LIBRARIES} ${ROOT_LIBRARIES})


add_executable(mininuclsims-GeomInfo ${PROJECT_SOURCE_DIR}/prog/mininuclsims-GeomInfo.cpp $<TARGET_OBJECTS:common_objs>)
target_link_libraries(mininuclsims-GeomInfo ${Geant4_LIBRARIES} ${ROOT_LIBRARIES})
#----------------------------------------------------------------------------
# Copy all scripts to the build directory, i.e. the directory in which we
# build the applications. This is so that we can run the executable directly because it
# relies on these scripts being in the current working directory.
#
##set(_SCRIPTS
  ##icons.mac
  ##gui.mac
  ##vis.mac
  ##)

##foreach(_script ${_SCRIPTS})
  ##configure_file(
    ##${PROJECT_SOURCE_DIR}/macros/${_script}
    ##${PROJECT_BINARY_DIR}/${_script}
    ##COPYONLY
    ##)
##endforeach()

#----------------------------------------------------------------------------
# Add program to the project targets
# (this avoids the need of typing the program name after make)
#
#add_custom_target(Xu2OptPh DEPENDS MITtut)

#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
#
#install(TARGETS ArgonCubeOptPh ArgonCubeOptPh-PVlist DESTINATION bin)
#install(FILES ${_SCRIPTS} DESTINATION bin)


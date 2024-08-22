### Project Settings ###
# Set min cmake version
if (WIN32)
    cmake_minimum_required(VERSION 3.21)
endif (WIN32)
if (UNIX)
    cmake_minimum_required(VERSION 3.5)
endif (UNIX)
cmake_policy(SET CMP0087 NEW)
# Set executable target depending on OS...
if (WIN32)
    set(PROJ_OUT "nchutils-test-windows")
    # Show console (FALSE=show, TRUE=don't show)
    set(CMAKE_WIN32_EXECUTABLE FALSE)
endif (WIN32)
if (UNIX)
    set(PROJ_OUT "nchutils-test-unix")
endif (UNIX)
# Project languages and flags
project(${PROJ_OUT} VERSION 0.1.1.0 LANGUAGES C CXX)
#set(CMAKE_C_FLAGS "-std=c++11 -O3 -DNDEBUG -s -flto")
set(CMAKE_C_FLAGS "-std=c++11")

### Create List of Sources, Headers, and Directories ###
# Get list of sources and headers
file (GLOB_RECURSE PROJ_SOURCES FOLLOW_SYMLINKS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp" "${CMAKE_SOURCE_DIR}/include/*.cpp")
file (GLOB_RECURSE PROJ_HEADERS FOLLOW_SYMLINKS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.h" "${CMAKE_SOURCE_DIR}/include/*.h")
# Build list of source directories based off of globbed headers
set (PROJ_SRC_DIRS "")
foreach (_headerFile ${PROJ_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND PROJ_SRC_DIRS ${_dir})
endforeach()
list (REMOVE_DUPLICATES PROJ_SRC_DIRS)


### Create Executable and add Links/Includes/DLLs ###
# Add executable
if (WIN32)
#    add_executable(${PROJ_OUT} WIN32 ${PROJ_SOURCES})
    add_executable(${PROJ_OUT} ${PROJ_SOURCES})
endif (WIN32)
if (UNIX)
    add_executable(${PROJ_OUT} ${PROJ_SOURCES})
endif (UNIX)
set_target_properties(${PROJ_OUT} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
# Add libraries + link them into target
add_library(PROJ_LIBS INTERFACE)
target_link_libraries(${PROJ_OUT} PUBLIC "-lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lavutil -lavformat -lavcodec -lswscale -lz -lm -lswresample")
get_target_property(TARGET_LIBS ${PROJ_OUT} LINK_LIBRARIES)
message("[NCH] Linked libraries: ${TARGET_LIBS}")
# Add include directories (everywhere there is a header file: libraries and PROJ src dirs)
target_include_directories(${PROJ_OUT} PRIVATE ${PROJ_SRC_DIRS})
target_include_directories(${PROJ_OUT} PRIVATE "include")

#add_custom_command(TARGET ${PROJ_OUT} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJ_OUT}> "${CMAKE_SOURCE_DIR}/bin"
#    COMMAND_EXPAND_LISTS
#)
#file(GENERATE OUTPUT filename CONTENT "content: $<TARGET_RUNTIME_DLLS:${PROJ_OUT}>")

message("[NCH] Finished config for ${PROJ_OUT}")
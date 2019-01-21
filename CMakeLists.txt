cmake_minimum_required(VERSION 3.7)
project(SDL_MusicPlayer)

include(GNUInstallDirs)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -Werror -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -ggdb -O2 -fno-omit-frame-pointer -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -DNDEBUG")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os -DNDEBUG")

find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libswscale)
# find_package(ffmpeg COMPONENTS avcodec avformat avutil swscale swresample)

link_directories(${FFMPEG_LIBRARY_DIRS})
set(LIBRARIES
    ${SDL2_LIBRARIES}
    ${FFMPEG_LIBRARIES}
)
set(INCLUDES
    include/
    ${SDL2_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
)

FILE(GLOB_RECURSE SOURCES "src/*.c")
FILE(GLOB INSTALL_HEADERS "include/*.h")

#add_library(SDL_MUSICSource SHARED ${SOURCES})
#add_library(SDL_MUSICSource_static STATIC ${SOURCES})

add_compile_options(${FFMPEG_CFLAGS_OTHER})
add_executable(${PROJECT_NAME} src/main.c)
#add_executable(${PROJECT_NAME} src/scale_video.c)

include_directories(${INCLUDES})
target_link_libraries(${PROJECT_NAME} ${FFMPEG_LDFLAGS_OTHER}) # It is OK to have link flags here
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

# Installation
install(FILES ${INSTALL_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/music_player)


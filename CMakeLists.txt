project(SDLDemo)
cmake_minimum_required(VERSION 3.0)

# Fix Ubuntu SDK IDE messing up C++ compilation...
string(REGEX REPLACE "gcc$" "g++" CMAKE_CXX_COMPILER "${CMAKE_CXX_COMPILER}")

# Determine if this is a build for the desktop kit. This is needed to
# pick the right SDL backend (typically there's no Mir when run in the
# desktop container) and to adapt the path where the resources are
# found (desktop apps are not click-packaged).
# Guess this by the hostname. The "desktop" kit seems to have hostname
# "desktop". This may be a bit fragile but I haven't found a better way.
execute_process(COMMAND hostname OUTPUT_VARIABLE HOSTNAME OUTPUT_STRIP_TRAILING_WHITESPACE)
if (HOSTNAME STREQUAL "desktop")
	message(STATUS "Building for the Desktop kit")
	set(HAVE_DESKTOP_KIT ON)
else()
	set(HAVE_DESKTOP_KIT OFF)
endif()

# Determine target triplet. Used when building SDL.
execute_process(
	COMMAND dpkg-architecture -qDEB_HOST_MULTIARCH
	OUTPUT_VARIABLE ARCH_TRIPLET
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Set C++ standard. The SDK containers currently have CMake 3.0.2 which
# does not support target property CXX_STANDARD and friends. So just pass
# it as compile option.
add_compile_options(-std=c++11)

# Do not remove this line, its required for the correct functionality of the Ubuntu-SDK
set(UBUNTU_MANIFEST_PATH "manifest.json.in" CACHE INTERNAL "Tells QtCreator location and name of the manifest file")

# Set install paths
set(CMAKE_INSTALL_PREFIX /)
set(DATA_DIR .)

# This command figures out the target architecture for use in the manifest file
execute_process(
	COMMAND dpkg-architecture -qDEB_HOST_ARCH
	OUTPUT_VARIABLE CLICK_ARCH
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(manifest.json.in ${CMAKE_CURRENT_BINARY_DIR}/manifest.json)
install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/manifest.json
	SDLDemo.desktop
	SDLDemo.apparmor
	DESTINATION .
)
install(DIRECTORY "media" DESTINATION ${DATA_DIR} PATTERN "media/src" EXCLUDE)

add_subdirectory(ext)
add_subdirectory(src)

# No op custom target for all not compiled files, so they show up in the QtCreator project tree
add_custom_target("SDLDemo_ClickFiles" ALL SOURCES "SDLDemo.desktop" "SDLDemo.apparmor" "manifest.json.in")

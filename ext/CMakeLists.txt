include(ExternalProject)

#
# SDL2
#

# SDL provides a CMakeLists.txt but I'm not sure how complete is it. So
# I used autotools build, which is the documented way of building SDL.

# Also note this builds the static library as shared library doesn't seem
# to bring any benefits in the click/snap scenario.

# The sources could be retrieved by ExternalProject but they are kept in
# mercurial and mercurial client is not available in the SDK containers.
# Instead just run download-dependencies.sh outside the container.

# To enable audio install libpulse-dev:$ARCH inside the containers, e.g.:
# apt install libpulse-dev:armhf

set(SDL2_SRC "${CMAKE_CURRENT_SOURCE_DIR}/SDL2")

if (NOT EXISTS ${SDL2_SRC})
	message(FATAL_ERROR "Can't find SDL2 sources. Please run download-dependencies.sh.")
endif ()

# Currently all kit containers include both X11 and Mir client libraries. But Mir
# SDL2 backend should be used on the phone while X11 backend should be used for
# the Desktop kit as by default no Mir is running.
if (HAVE_DESKTOP_KIT)
	set(USE_MIR OFF)
else()
	set(USE_MIR ON)
endif()

if (USE_MIR)
	message(STATUS "Building SDL2 with Mir backend")

	set(SDL2_VIDEO_BACKEND_OPTIONS
		--enable-video-mir
		--disable-mir-shared # no need for "soft" dependency
		--disable-video-x11
	)

	find_library(LIBMIRCLIENT mirclient)
	if (NOT LIBMIRCLIENT)
		message(FATAL_ERROR "Can't find libmirclient")
	endif()

	set(SDL2_VIDEO_BACKEND_LINK_LIBRARIES "${LIBMIRCLIENT}")
else(USE_MIR)
	message(STATUS "Building SDL2 with X11 backend")

	# X11 simulation of the phone environment
	set(SDL2_VIDEO_BACKEND_OPTIONS
		--disable-video-mir
		--enable-video-x11
		--disable-x11-shared # no need for "soft" dependency
		# ... also disable a bunch of extensions not needed for a simple PC build
		--disable-video-x11-xcursor
		--disable-video-x11-xdbe
		--disable-video-x11-xinerama
		--disable-video-x11-xrandr
		--disable-video-x11-scrnsaver
		--disable-video-x11-xshape
		--disable-video-x11-vm
		# enable this - if you have a touchscreen this should work like on the phone
		# ... but this requires the libxi-dev package in the kit
		# if libxi is not found, SDL's configure will ignore this option
		--enable-video-x11-xinput
	)

	find_library(LIBX11 X11)
	find_library(LIBXEXT Xext)

	if (NOT LIBX11)
		message(FATAL_ERROR "Can't find libX11")
	endif()
	if (NOT LIBXEXT)
		message(FATAL_ERROR "Can't find libXext")
	endif()

	set(SDL2_VIDEO_BACKEND_LINK_LIBRARIES "${LIBX11};${LIBXEXT}")

	find_library(LIBXI Xi)
	if (LIBXI)
		list(APPEND SDL2_VIDEO_BACKEND_LINK_LIBRARIES "${LIBXI}")
	else ()
		message(WARNING "libXi (X input) not found, building without touch support; install libxi-dev in the container if you want touch support with X11")
	endif ()
endif(USE_MIR)

# Attempt to use the target-specific pkg-config but if it's not available,
# fall back to "pkg-config". (This actually happens with the desktop kit.)
find_program(PKG_CONFIG NAMES ${ARCH_TRIPLET}-pkg-config pkg-config)

ExternalProject_Add(SDL2_build
	URL ${SDL2_SRC}
	# Override the PKG_CONFIG variable so that SDL's configure script uses the right
	# pkg-config. (See https://bugzilla.libsdl.org/show_bug.cgi?id=3575)
	# Use the same C and C++ compiler that CMake uses, specify host for possible
	# cross compilation, enable features we need and disable some of the things
	# we don't need.
	CONFIGURE_COMMAND PKG_CONFIG=${PKG_CONFIG} <SOURCE_DIR>/configure
		CC=${CMAKE_C_COMPILER}
		CXX=${CMAKE_CXX_COMPILER}
		--host=${ARCH_TRIPLET}
		--prefix=<INSTALL_DIR>
		--disable-shared
		--enable-static # static makes more sense for Click/Snappy
		--disable-atomic # thanks, but we've got std::atomic
		--disable-joystick # not of much use on the phone
		--disable-video-dummy
		--disable-video-opengl
		--disable-video-opengles1
		--enable-video-opengles2
		${SDL2_VIDEO_BACKEND_OPTIONS}
		--disable-dbus
		--disable-ibus
		--disable-diskaudio
		--disable-dummyaudio
		--disable-oss
		--disable-alsa
		--disable-esd
		--disable-arts
		--disable-nas
		--disable-sndio
		--enable-pulseaudio
		--disable-pulseaudio-shared # no need to load at runtime, pulseaudio is always available on the phone
	BUILD_COMMAND make
	INSTALL_COMMAND make install
)
ExternalProject_Get_Property(SDL2_build INSTALL_DIR)
set(SDL2_INSTALL_DIR ${INSTALL_DIR})

# directories where SDL2 stuff is installed

# There is some fuzzyness whether the SDL2 headers should be included
# via:
#     #include <SDL2/SDL.h>
# or just:
#     #include <SDL.h>
#
# The first approach needs the ${SDL2_INSTALL_DIR}/include to be added to
# include paths, the ther needs ${SDL2_INSTALL_DIR}/include/SDL2.
#
# SDL2 tutorials seem to prefer the second way. SDL2pp enforces the
# first way.
#
# This issue is exacerbated when SDL_image, SDL_ttf or SDL_mixer are
# installed into different directories. SDL_image, SDL_ttf and SDL_mixer
# include the SDL headers via #include <SDL.h> or siblings. This always
# works if they are located in the same directories but they are
# installed into different directories in this project.
#
# This means here both directories need to be added to include paths:
# * ${SDL2_INSTALL_DIR}/include - for SDL2pp to work
# * ${SDL2_INSTALL_DIR}/include/SDL2 - for SDL_* to work
#
# There is a bug ticket for this in SDL2pp:
# https://github.com/libSDL2pp/libSDL2pp/issues/88
set(SDL2_HEADERS "${SDL2_INSTALL_DIR}/include/SDL2")
set(SDL2_HEADERS_SDL2PP "${SDL2_INSTALL_DIR}/include") # used for SDL2pp
set(SDL2_PKG_CONFIG_PATH "${SDL2_INSTALL_DIR}/lib/pkgconfig")

# The header install directory is normally created by SDL's install step
# but if the directory doesn't exist at CMake configuration and it's
# passed to INTERFACE_INCLUDE_DIRECTORIES, CMake considers it an error.
# So just make it now and let the install step populate it later.
file(MAKE_DIRECTORY "${SDL2_HEADERS}" "${SDL2_HEADERS_SDL2PP}")

add_library(SDL2 SHARED IMPORTED GLOBAL)

# SDL has some dependencies. But as we build a static library outside of
# CMake, CMake can't tell what these are. SDL comes with pkg-config file
# but it's installed only after SDL is build and installed. This is too
# late - we need the dependencies at configure time. So just hardcode them
# here. (They should match what sdl2.pc ends up listing.)

find_library(LIBM m)
find_library(LIBDL dl)
find_library(LIBPTHREAD pthread)
find_library(LIBRT rt)
find_library(LIBXKBCOMMON xkbcommon)
find_library(LIBPULSE pulse)
find_library(LIBPULSE_SIMPLE pulse-simple)

if (NOT LIBM)
	message(FATAL_ERROR "Can't find libm.")
endif ()
if (NOT LIBDL)
	message(FATAL_ERROR "Can't find libdl.")
endif ()
if (NOT LIBPTHREAD)
	message(FATAL_ERROR "Can't find libpthread.")
endif ()
if (NOT LIBRT)
	message(FATAL_ERROR "Can't find librt.")
endif ()
if (NOT LIBXKBCOMMON)
	message(FATAL_ERROR "Can't find libxkbcommon.")
endif ()
if (NOT LIBPULSE)
	message(FATAL_ERROR "Can't find libpulse.")
endif ()
if (NOT LIBPULSE_SIMPLE)
	message(FATAL_ERROR "Can't find libpulse-simple.")
endif ()

set_target_properties(SDL2 PROPERTIES
	IMPORTED_LOCATION "${SDL2_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}SDL2${CMAKE_STATIC_LIBRARY_SUFFIX}"
	INTERFACE_INCLUDE_DIRECTORIES "${SDL2_HEADERS}"
	IMPORTED_LINK_INTERFACE_LIBRARIES "${LIBM};${LIBDL};${LIBPTHREAD};${LIBRT};${SDL2_VIDEO_BACKEND_LINK_LIBRARIES};${LIBXKBCOMMON};${LIBPULSE};${LIBPULSE_SIMPLE}"
)
add_dependencies(SDL2 SDL2_build)

#
# SDL2_image
#

set(SDL2_IMAGE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_image")

if (NOT EXISTS ${SDL2_IMAGE_SRC})
	message(FATAL_ERROR "Can't find SDL2_image sources. Please run download-dependencies.sh.")
endif ()

ExternalProject_Add(SDL2_IMAGE_build
	DEPENDS SDL2
	URL ${SDL2_IMAGE_SRC}
	# It seems that the timestamps in the HG tag for SDL2_image that is used here
	# are wrong: Autotools generated files are older than source files even though
	# the generated files seem to be out of date. One option is to regenerate them
	# again, but this seems to fail in the conainer. As a work-around just run
	# autogen.sh even though this should not be necessary if the timestamps were OK.
	PATCH_COMMAND ./autogen.sh
	# add SDL_PKG_CONFIG_PATH to PKG_CONFIG_PATH so that SDL2_image can find SDL2
	# (and make sure it's the first item in case there already was other SDL2
	# build available)
	CONFIGURE_COMMAND PKG_CONFIG_PATH=${SDL2_PKG_CONFIG_PATH}:$ENV{PKG_CONFIG_PATH} <SOURCE_DIR>/configure
		CC=${CMAKE_C_COMPILER}
		CXX=${CMAKE_CXX_COMPILER}
		--host=${ARCH_TRIPLET}
		--prefix=<INSTALL_DIR>
		--disable-shared
		--enable-static
		--disable-bmp
		--disable-gif
		--disable-jpg
		--disable-lbm
		--disable-pcx
		--enable-png
		--disable-png-shared # don't load at runtime, just depend on it
		--disable-pnm
		--disable-tga
		--disable-tif
		--disable-xcf
		--disable-xpm
		--disable-xv
		--disable-webp
	BUILD_COMMAND make
	INSTALL_COMMAND make install
)
ExternalProject_Get_Property(SDL2_IMAGE_build INSTALL_DIR)
set(SDL2_IMAGE_INSTALL_DIR ${INSTALL_DIR})

# directories where SDL2_IMAGE stuff is installed
set(SDL2_IMAGE_HEADERS "${SDL2_IMAGE_INSTALL_DIR}/include/SDL2")
set(SDL2_IMAGE_HEADERS_SDL2PP "${SDL2_IMAGE_INSTALL_DIR}/include") # used for SDL2pp

file(MAKE_DIRECTORY "${SDL2_IMAGE_HEADERS}" "${SDL2_IMAGE_HEADERS_SDL2PP}")

add_library(SDL2_IMAGE SHARED IMPORTED GLOBAL)

find_library(LIBPNG png)

set_target_properties(SDL2_IMAGE PROPERTIES
	IMPORTED_LOCATION "${SDL2_IMAGE_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}SDL2_image${CMAKE_STATIC_LIBRARY_SUFFIX}"
	INTERFACE_INCLUDE_DIRECTORIES "${SDL2_IMAGE_HEADERS}"
	IMPORTED_LINK_INTERFACE_LIBRARIES "${LIBPNG}"
)
add_dependencies(SDL2_IMAGE SDL2_IMAGE_build)

#
# SDL2_ttf
#
# Requires freetype, install inside the containers via:
# apt install libfreetype6-dev:armhf (or whatever your target arch is)

set(SDL2_TTF_SRC "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_ttf")

if (NOT EXISTS ${SDL2_TTF_SRC})
	message(FATAL_ERROR "Can't find SDL2_ttf sources. Please run download-dependencies.sh.")
endif ()

ExternalProject_Add(SDL2_TTF_build
	DEPENDS SDL2
	URL ${SDL2_TTF_SRC}
	# Disable some test programs SDL2_ttf unconditionally builds.
	PATCH_COMMAND patch -p1 < "${CMAKE_CURRENT_SOURCE_DIR}/patches/sdl2_ttf-no-programs.patch" && ./autogen.sh
	# add SDL_PKG_CONFIG_PATH to PKG_CONFIG_PATH so that SDL2_ttf can find SDL2
	# (and make sure it's the first item in case there already was other SDL2
	# build available)
	CONFIGURE_COMMAND PKG_CONFIG_PATH=${SDL2_PKG_CONFIG_PATH}:$ENV{PKG_CONFIG_PATH} <SOURCE_DIR>/configure
		CC=${CMAKE_C_COMPILER}
		CXX=${CMAKE_CXX_COMPILER}
		--host=${ARCH_TRIPLET}
		--prefix=<INSTALL_DIR>
		--disable-shared
		--enable-static
	BUILD_COMMAND make
	INSTALL_COMMAND make install
)
ExternalProject_Get_Property(SDL2_TTF_build INSTALL_DIR)
set(SDL2_TTF_INSTALL_DIR ${INSTALL_DIR})

# directories where SDL2_TTF stuff is installed
set(SDL2_TTF_HEADERS "${SDL2_TTF_INSTALL_DIR}/include/SDL2")
set(SDL2_TTF_HEADERS_SDL2PP "${SDL2_TTF_INSTALL_DIR}/include") # used for SDL2pp

file(MAKE_DIRECTORY "${SDL2_TTF_HEADERS}" "${SDL2_TTF_HEADERS_SDL2PP}")

add_library(SDL2_TTF SHARED IMPORTED GLOBAL)

find_library(LIBFREETYPE freetype)

set_target_properties(SDL2_TTF PROPERTIES
	IMPORTED_LOCATION "${SDL2_TTF_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}SDL2_ttf${CMAKE_STATIC_LIBRARY_SUFFIX}"
	INTERFACE_INCLUDE_DIRECTORIES "${SDL2_TTF_HEADERS}"
	IMPORTED_LINK_INTERFACE_LIBRARIES "${LIBFREETYPE}"
)
add_dependencies(SDL2_TTF SDL2_TTF_build)

#
# SDL2_mixer
#

set(SDL2_MIXER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_mixer")

if (NOT EXISTS ${SDL2_MIXER_SRC})
	message(FATAL_ERROR "Can't find SDL2_mixer sources. Please run download-dependencies.sh.")
endif ()

ExternalProject_Add(SDL2_MIXER_build
	DEPENDS SDL2
	URL ${SDL2_MIXER_SRC}
	# add SDL_PKG_CONFIG_PATH to PKG_CONFIG_PATH so that SDL2_mixer can find SDL2
	# (and make sure it's the first item in case there already was other SDL2
	# build available)
	CONFIGURE_COMMAND PKG_CONFIG_PATH=${SDL2_PKG_CONFIG_PATH}:$ENV{PKG_CONFIG_PATH} <SOURCE_DIR>/configure
		CC=${CMAKE_C_COMPILER}
		CXX=${CMAKE_CXX_COMPILER}
		--host=${ARCH_TRIPLET}
		--prefix=<INSTALL_DIR>
		--disable-shared
		--enable-static
	BUILD_COMMAND make
	INSTALL_COMMAND make install
)
ExternalProject_Get_Property(SDL2_MIXER_build INSTALL_DIR)
set(SDL2_MIXER_INSTALL_DIR ${INSTALL_DIR})

# directories where SDL2_mixer stuff is installed
set(SDL2_MIXER_HEADERS "${SDL2_MIXER_INSTALL_DIR}/include")
set(SDL2_MIXER_HEADERS_SDL2PP "${SDL2_MIXER_INSTALL_DIR}/include") # used for SDL2pp

file(MAKE_DIRECTORY "${SDL2_MIXER_HEADERS}" "${SDL2_MIXER_HEADERS_SDL2PP}")

add_library(SDL2_MIXER SHARED IMPORTED GLOBAL)

set_target_properties(SDL2_MIXER PROPERTIES
	IMPORTED_LOCATION "${SDL2_MIXER_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}SDL2_mixer${CMAKE_STATIC_LIBRARY_SUFFIX}"
	INTERFACE_INCLUDE_DIRECTORIES "${SDL2_MIXER_HEADERS}"
	IMPORTED_LINK_INTERFACE_LIBRARIES "${LIBM}"
)
add_dependencies(SDL2_MIXER SDL2_MIXER_build)

#
# SDL2pp
#

set(SDL2pp_SRC "${CMAKE_CURRENT_SOURCE_DIR}/SDL2pp")

if (NOT EXISTS ${SDL2_SRC})
	message(FATAL_ERROR "Can't find SDL2pp sources. Please run download-dependencies.sh.")
endif ()

ExternalProject_Add(SDL2pp_build
	DEPENDS SDL2 SDL2_IMAGE SDL2_TTF SDL2_MIXER
	URL ${SDL2pp_SRC}
	# patch #1: SDL2pp currently builds only shared library in standalone mode but we want static
	# patch #2: fix include paths when SDL_image is installed into a directory other than where SDL itself is installed
	PATCH_COMMAND patch -p1 < "${CMAKE_CURRENT_SOURCE_DIR}/patches/sdl2pp-make-lib-static.patch" &&
		patch -p1 < "${CMAKE_CURRENT_SOURCE_DIR}/patches/sdl2pp-sdl-img-ttf-different-dirs.patch"
	# due to the way cmake handles ';' in strings (lists), use ':' and let ExternalProject
	# replace these by ';'
	LIST_SEPARATOR ":"
	CONFIGURE_COMMAND cmake <SOURCE_DIR>
		# These two would be set by MultiArchCross.cmake, but if they were overridden...
		-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
		-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
		# Paths to the SDL2 + helper libs we built
		-DCMAKE_INCLUDE_PATH=${SDL2_HEADERS_SDL2PP}:${SDL2_IMAGE_HEADERS_SDL2PP}:${SDL2_TTF_HEADERS_SDL2PP}:${SDL2_MIXER_HEADERS_SDL2PP}
		-DCMAKE_LIBRARY_PATH=${SDL2_INSTALL_DIR}/lib:${SDL2_IMAGE_INSTALL_DIR}/lib:${SDL2_TTF_INSTALL_DIR}/lib:${SDL2_MIXER_INSTALL_DIR}/lib
		# SDL2pp options
		-DSDL2PP_WITH_IMAGE=ON
		-DSDL2PP_WITH_TTF=ON
		-DSDL2PP_WITH_MIXER=ON
		-DSDL2PP_WITH_EXAMPLES=OFF
		-DSDL2PP_WITH_TESTS=OFF
		-DSDL2PP_ENABLE_LIVE_TESTS=OFF
		# Where to install
		-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
	BUILD_COMMAND make
	INSTALL_COMMAND make install
)
ExternalProject_Get_Property(SDL2pp_build INSTALL_DIR)
set(SDL2PP_INSTALL_DIR ${INSTALL_DIR})

add_library(SDL2pp SHARED IMPORTED GLOBAL)
set(SDL2PP_HEADERS "${SDL2PP_INSTALL_DIR}/include")
file(MAKE_DIRECTORY "${SDL2PP_HEADERS}")
set_target_properties(SDL2pp PROPERTIES
	IMPORTED_LOCATION "${SDL2PP_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}SDL2pp${CMAKE_STATIC_LIBRARY_SUFFIX}"
	# SDL2_*HEADERS_SDL2PP required to make the #includes from SDL2pp to SDL2 work
	INTERFACE_INCLUDE_DIRECTORIES "${SDL2PP_HEADERS};${SDL2_HEADERS_SDL2PP};${SDL2_IMAGE_HEADERS_SDL2PP};${SDL2_TTF_HEADERS_SDL2PP};${SDL2_MIXER_HEADERS_SDL2PP}"
	IMPORTED_LINK_INTERFACE_LIBRARIES "SDL2;SDL2_IMAGE;SDL2_TTF;SDL2_MIXER"
)

add_dependencies(SDL2pp SDL2pp_build)

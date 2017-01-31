# Currently the Ubuntu SDK target containers lack boost devel packages.
# In order for the following command to succeed it's necessary
# to install at least libboost-filesystem-dev and libboost-system-dev
# for the target arch. For example, for armhf, run:
# $ usdk-target maint device-armhf apt install libboost-filesystem-dev:armhf libboost-system-dev:armhf
find_package(Boost REQUIRED COMPONENTS filesystem system)

add_executable(sdl_demo
	SDLDemo.cpp
	ResourcePath.h
	ResourcePath.cpp
	FingerImage.h
	FingerImage.cpp
)
target_include_directories(sdl_demo PRIVATE ${Boost_INCLUDE_DIRS})

if (HAVE_DESKTOP_KIT)
	# When run in the "desktop" kit the application is not packaged and is
	# run from the CMake build directory, see e.g.:
	# https://bazaar.launchpad.net/~ubuntu-sdk-team/qtcreator-plugin-ubuntu/trunk/view/470/src/ubuntu/device/container/ubuntulocalrunconfiguration.cpp?#L140
	#
	# Also, because of https://bugs.launchpad.net/ubuntu/+source/qtcreator-plugin-ubuntu/+bug/1667119
	# the working directory is quite useless - it's the user's home directory.
	#
	# As a work-around, pass the source directory via a #define to the compilation.
	# This way it can be used in the sources to hardcode resources search path.
	target_compile_definitions(sdl_demo PRIVATE
		HAVE_DESKTOP_KIT
		RESOURCES_BASE_DIRECTORY="${CMAKE_SOURCE_DIR}"
	)
endif()

target_link_libraries(sdl_demo SDL2pp ${Boost_LIBRARIES})

install(TARGETS sdl_demo RUNTIME DESTINATION .)

#!/bin/bash
set -e

# change to the directory where the script is stored
cd `dirname $0`

# Download all the external dependencies. They are then built using
# CMake's ExternalProject module. The module could download the
# dependencies too, but it needs e.g. mercurial to get the SDL source
# and mercurial is not available in the Ubuntu SDK containers by default.
# So I created this shell script. Run it once, outside of the chroot.
# You still need mercurial for this.

if [ -e SDL2 ]; then
	echo "Skipping SDL2 because it's been downloaded already."
else
	# The last revision that supports Mir 0.24. The official SDL2 release
	# 2.0.5 already requires Mir 0.25 which is currently not availables in
	# the images.
	echo "Downloading SDL2..."
	hg clone -r 10361 http://hg.libsdl.org/SDL SDL2
fi

if [ -e SDL2_image ]; then
	echo "Skipping SDL2_image because it's been downloaded already."
else
	echo "Downloading SDL2_image..."
	hg clone -r release-2.0.1 https://hg.libsdl.org/SDL_image SDL2_image
fi

if [ -e SDL2_ttf ]; then
	echo "Skipping SDL2_image because it's been downloaded already."
else
	echo "Downloading SDL2_image..."
	hg clone -r release-2.0.14 http://hg.libsdl.org/SDL_ttf/ SDL2_ttf
fi

if [ -e SDL2_mixer ]; then
	echo "Skipping SDL2_mixer because it's been downloaded already."
else
	echo "Downloading SDL2_mixer..."
	hg clone -r release-2.0.1 http://hg.libsdl.org/SDL_mixer/ SDL2_mixer
fi

if [ -e SDL2pp ]; then
	echo "Skipping SDL2pp because it's been downloaded already."
else
	echo "Downloading SDL2pp..."

	# shallow clone tag 0.13.0
	git clone --depth 1 --branch 0.13.0 https://github.com/libSDL2pp/libSDL2pp.git SDL2pp
fi

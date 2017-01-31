/*
 * SDLDemo - SDL2 Demo project for the Ubuntu SDK.
 *
 * Written in 2017 by David Kozub <zub@linux.fjfi.cvut.cz>
 *
 * To the extent possible under law, the author(s) have dedicated all
 * copyright and related and neighboring rights to this software to the
 * public domain worldwide. This software is distributed without any
 * warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication
 * along with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
#include "ResourcePath.h"

#include <SDL.h>
#include <iostream>

using boost::filesystem::path;

static path resourcePathRoot;

void initResourcePath()
{
	// let CMake override the root of the base path via #defines
	const char *basePath =
#ifndef RESOURCES_BASE_DIRECTORY
		// the default - used on the phone
		SDL_GetBasePath()
#else
	/**
	 * For the desktop kit the application is not packaged and is run directly
	 * from the CMake build directory. So let CMake define
	 * RESOURCES_BASE_DIRECTORY to where the sources actually are.
	 */
		RESOURCES_BASE_DIRECTORY
#endif
	;

	resourcePathRoot = path(basePath) / "media";

	std::cout << "Using resource base path: " << basePath << std::endl;
}

path getResourcePath(const std::string& component)
{
	return resourcePathRoot / component;
}

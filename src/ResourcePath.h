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
#ifndef RESOURCE_PATH_H
#define RESOURCE_PATH_H

#include <string>
#include <boost/filesystem.hpp>

void initResourcePath();
boost::filesystem::path getResourcePath(const std::string& resource);

#endif // RESOURCE_PATH_H

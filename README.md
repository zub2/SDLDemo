# SDL2 Demo Project for Ubuntu SDK

This is a template for a native C++ application using [SDL2](https://www.libsdl.org/), [SDL_image](https://www.libsdl.org/projects/SDL_image/), [SDL_ttf](https://www.libsdl.org/projects/SDL_ttf/), [SDL_mixer](https://www.libsdl.org/projects/SDL_mixer/) and [SDL2pp](https://github.com/libSDL2pp/libSDL2pp) built using [Ubuntu SDK](https://developer.ubuntu.com/en/phone/platform/sdk/).

The project has been tested with Ubuntu SDK 15.04 image, which is what is currently available for the MX4 phone as of this writing (February 2017).

## Why Another SDL2 Template Project?
There are at least the following SDL2 templates on Github: [ubuntu-touch-sdl-template](https://github.com/Sturmflut/ubuntu-touch-sdl-template) and [ubuntu-touch-sdl2-gles2-template](https://github.com/Sturmflut/ubuntu-touch-sdl2-gles2-template). But they don't integrate SDL2 build into CMake. You need to build SDL yourself. I wanted a project that builds SDL automatically.

Included are the commonly used libraries SDL_image, SDL_ttf and SDL_mixer, and a C++ wrapper (I chose SDL2pp).

I tuned the configuration of SDL and the other libraries to the Ubuntu Touch: Enabled are only the audio and video backends that are used on the system and e.g. Mir support is unconditional (no on-demand loading) because Mir is always available. It is possible to tune this further depending on your project's needs. You can for example disable unneeded subsystems. (See `etc/CMakeLists.txt`.)

Static libraries are built because using a shared library inside a click/snappy package doesn't make much sense (unless your package contains multiple executables that would share the library). This could have some licensing consequences but to the extent of my knowlegde ([IANAL](https://en.wiktionary.org/wiki/IANAL)) using statically-linked SDL in a GPLv2 or GPLv3 application is [allowed](https://choosealicense.com/licenses/zlib/).

## What is included?
Included are:
* [SDL2](https://www.libsdl.org/) (covered by a zlib-style license)
* [SDL_image](https://www.libsdl.org/projects/SDL_image/) (covered by a zlib-style license)
* [SDL_ttf](https://www.libsdl.org/projects/SDL_ttf/) (covered by a zlib-style license)
* [SDL_mixer](https://www.libsdl.org/projects/SDL_mixer/) (covered by a zlib-style license)
* [SDL2pp](http://sdl2pp.amdmi3.ru/) (covered by a zlib-style license)
* some simple demo code that uses all the libraries (covered by [CC0 1.0](http://creativecommons.org/publicdomain/zero/1.0/) to make it easy to use for any purpose)
* some media files used by the demo to demonstrate font rendering and sound playback:
   * [FreeSans](http://ftp.gnu.org/gnu/freefont/freefont-otf-20120503.tar.gz) font from [Gnu FreeFont](https://www.gnu.org/software/freefont/) (covered by [GNU GPLv3](http://www.gnu.org/licenses/))
   * [Blip C 02](http://www.freesound.org/people/cabled_mess/sounds/350864/) and [Lose C 02](http://www.freesound.org/people/cabled_mess/sounds/350985/) sounds by [cabled_mess](http://www.freesound.org/people/cabled_mess/) (covered by [CC0 1.0](http://creativecommons.org/publicdomain/zero/1.0/))

Currently SDL revision 10349 is used (this is a revision between SDL versions 2.0.4 and 2.0.5). The reason for this is that this is the last revision that supports Mir 0.24 which is what is currently (February 2017) available for Ubuntu Phone. Newer revisions [don't build with Mir 0.24](https://hg.libsdl.org/SDL/rev/070fe7c1fc85). The Mir API is not too stable so when the Mir version changes a different SDL version likely needs to be used.

## What does the demo actually do?
If you run it you should see a black screen. If you touch the window anywhere an image centered on the contact position should show up. Multiple fingers are supported (provided the screen can handle it). Mouse is also handled so the demo can be tested even on the PC without touch support. Theoretically both touch and mouse should work at the same time.

I currently don't have a device with X11 and touch so I have not tested touch with X11.

## Desktop Kit
The desktop kit turns out to be a bit different from the other kits. For example Mir is not available. Another difference is that applications are not packaged prior to running them - the application is started directly form the CMake build directory. Then there are smaller differences like screen DPI and the fact that on the phone your application should be fullscreen while this may not be true on the desktop.
So in order to support build for the desktop kit the following important things must be handled:
* build SDL2 with the X11 backed
* let the C++ code know that this is a build for the desktop kit - this demo uses `HAVE_DESKTOP_KIT` define (see the top-level `CMakeLists.txt` and `src/CMakeLists.txt`)
* let the C++ code know the location of the source directory so that it can still find resources (images, fonts, sounds) - this demo uses RESOURCES_BASE_DIRECTORY (see `src/CMakeLists.txt`)

With these things handled the demo runs with the desktop kit. The images are too big due to the DPI difference between a phone and a typical PC display, but fixing this is out of scope of this demo.

## How to Use it
### Prerequisites
There are some things that need to be done prior to actually building the project. These inclue:
* (surprise!) Get Ubuntu SDK installed and running.
* Make sure you have Mercurial and Git installed on your computer. (They don't have to be installed inside the Ubuntu SDK containers.) You can verify this by running: `apt install mercurial git`
* Install required development packages in each of the Ubuntu SDK containers.

The following packages are needed:
* libpulse-dev (for audio support in SDL)
* libfreetype6-dev (for font rendering support in SDL_ttf)
* libboost-filesystem-dev and libboost-system-dev (used by the demo, not needed for SDL)
* (only for the desktop kit) libxi-dev if you want touch support

These libraries are available on the phone but the SDK containers don't come with the _-dev_ packages needed to use them installed. Nevertheless it is possible to install these via `apt`:
1. Start Ubuntu SDK IDE.
1. Select _Tools/Options..._ in the menu.
1. Select _Ubuntu_ in the list on the left-hand side.
1. Select tab _Click_.
1. Do the following for each configured kit:
   1. Make sure it's up to date by clicking _Update_. Not all libraries were available in the original container versions.
   1. Click _Maintain_. A terminal window pops up.
   1. Run the following command:
      * for _armhf_: `apt install libpulse-dev:armhf libfreetype6-dev:armhf libboost-filesystem-dev:armhf libboost-system-dev:armhf`
      * for _i386_: `apt install libpulse-dev:i386 libfreetype6-dev:i386 libboost-filesystem-dev:i386 libboost-system-dev:i386`
      * for _desktop_: `apt install libpulse-dev libfreetype6-dev libboost-filesystem-dev libboost-system-dev libxi-dev`

### Building the SDL2 Demo project
With the prerequisites sorted out the rest is quite easy:
1. Clone this project.
1. Download the sources for external libraries. To do that, open a terminal, go to the root of the project and run `etc/download-dependencies.sh` (Alternatively, CMake could do this step automatically but this would mean mercurial and git need to be installed in the containers, plus making this step explicit hopefully spares some unnecessary re-downloading.)
1. Start Ubuntu SDK and open the project. To do that, select the menu item _File_/_Open File or Project..._ and choose the `CMakeLists.txt` file in the root folder of the project. Select the kits you want. (You can select all kits for which you installed the necessary _-dev_ packages. Do _not_ select the kit named just _Desktop_. The kit _UbuntuSDK for desktop_ can be selected.)
1. Ubuntu SDK IDE runs CMake for the first selected kit. If the required _-dev_ packages were installed correctly, CMake should not produce an error message.
1. Select the kit you want and build the project (_Build_/_Build Project "SDLDemo"_). This takes some time. You shoudl see the various libraries being built in the _Compile Output_ pane. This should eventually finish without any errors.
1. Select the start up configuration: Click on the Ubuntu icon in the left bar of Ubuntu SDK IDE. In the menu that appears, in the rightmost column (_Run_), select _SDLDemo_. (This is the correct configuration that can be used to launch the application in either the emulator, on the local PC or on a real phone. The name comes from `mainfest.json.in`, from the hooks array. The other option there, _sdl_demo_, comes from the name of the actual CMake executable target and it's probably picked up by the default QtCreator CMake plugin. This configuration can't be used to launch the application in the emulator or on the phone.)
1. Run, touch the window or click it with a mouse and enjoy. :)

## Tuning the SDL2 configuration
The build for all the bundled libraries is configured in `ext/CMakeLists.txt`.

## License
As already mentioned above, the code (including the actual C++ code of the demo and the CMake files) in this project is licensed under the [CC0 1.0](http://creativecommons.org/publicdomain/zero/1.0/) to make it easy to reuse in any way. But the actual libraries and the FreeSans font used in the projct come with licenses that are _not_ the CC0!

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
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <cstdint>
#include <map>
#include <algorithm>
#include <random>
#include <boost/filesystem.hpp>

#include <SDL.h>
#include <SDL_touch.h>
#include <SDL_image.h>

#include <SDL2pp/SDL.hh>
#include <SDL2pp/SDLTTF.hh>
#include <SDL2pp/SDLMixer.hh>
#include <SDL2pp/Window.hh>
#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Mixer.hh>
#include <SDL2pp/Chunk.hh>

#include "ResourcePath.h"
#include "FingerImage.h"

class SDLDemo
{
public:
	static void execute(std::random_device::result_type randomSeed);

private:
	struct Finger
	{
		Finger(size_t colorIndex, FingerImage&& fingerImage):
			colorIndex(colorIndex),
			fingerImage(std::move(fingerImage))
		{}

		size_t colorIndex;
		FingerImage fingerImage;
	};

	/*
	 * To be able to combine both finger IDs and mouse devices (possibly more of them)
	 * into one map, use (bool, SDL_FingerID) as the key. The bool is true for touch
	 * events and false for mouse event.
	 * This assumes that the finger IDs are unique across all touch devices, which I'm
	 * not sure about, but I couldn't find the answer for this. If it's not, SDL_TouchId
	 * would have to be added to distinguish between different touch devices.
	 */
	typedef std::pair<bool, SDL_FingerID> FingerID;
	typedef std::map<FingerID, Finger> FingerMap;

	static SDL2pp::Window createWindow();

	SDLDemo(std::random_device::result_type randomSeed);
	void run();
	void redraw();
	void handleEvent(const SDL_Event& event);

	SDL2pp::Point getFingerPosition(const SDL_TouchFingerEvent& touchEvent);
	void handleFingerDown(const SDL_TouchFingerEvent& touchEvent);
	void handleFingerMotion(const SDL_TouchFingerEvent& touchEvent);
	void handleFingerUp(const SDL_TouchFingerEvent& touchEvent);

	void handleMouseButtonDown(const SDL_MouseButtonEvent& mouseButtonEvent);
	void handleMouseMotion(const SDL_MouseMotionEvent& mouseMotionEvent);
	void handleMouseButtonUp(const SDL_MouseButtonEvent& mouseButtonEvent);

	void fingerNew(bool isTouch, SDL_FingerID fingerId, const SDL2pp::Point& position);
	void fingerMove(bool isTouch, SDL_FingerID fingerId, const SDL2pp::Point& position);
	void fingerDelete(bool isTouch, SDL_FingerID fingerId);
	FingerMap::iterator findFinger(bool isTouch, SDL_FingerID fingerId);

	size_t pickColor();

	void playSound(const SDL2pp::Chunk& chunk);

	// as the creation is done directly in the initializer the order here is important
	SDL2pp::Window m_window;
	SDL2pp::Renderer m_renderer;
	const int m_windowX;
	const int m_windowY;
	bool m_quit;
	bool m_sceneChanged;
	std::default_random_engine m_randomEngine;
	SDL2pp::Mixer m_mixer;
	SDL2pp::Chunk m_soundFingerDown;
	SDL2pp::Chunk m_soundFingerUp;

	FingerMap m_fingerImages;

	static constexpr size_t COLOR_COUNT = 10;

	size_t m_colorUsage[COLOR_COUNT];
	static const SDL_Color COLORS[COLOR_COUNT];

	static const char* SOUND_FINGER_DOWN_FILE;
	static const char* SOUND_FINGER_UP_FILE;
};

const SDL_Color SDLDemo::COLORS[SDLDemo::COLOR_COUNT] =
{
	SDL_Color{173, 68, 207, 255},
	SDL_Color{101, 255, 51, 255},
	SDL_Color{71, 168, 201, 255},
	SDL_Color{207, 104, 68, 255},
	SDL_Color{61, 70, 166, 255},
	SDL_Color{222, 67, 149, 255},
	SDL_Color{222, 217, 67, 255},
	SDL_Color{40, 148, 44, 255},
	SDL_Color{100, 237, 228, 255},
	SDL_Color{255, 255, 255, 255}
};

const char* SDLDemo::SOUND_FINGER_DOWN_FILE = "350864__cabled-mess__blip-c-02.wav";
const char* SDLDemo::SOUND_FINGER_UP_FILE = "350985__cabled-mess__lose-c-02.wav";

void SDLDemo::execute(std::random_device::result_type randomSeed)
{
	SDLDemo demo(randomSeed);
	demo.run();
}

SDL2pp::Window SDLDemo::createWindow()
{
#ifdef HAVE_DESKTOP_KIT
	/*
	 * Building with the desktop kit. There is (typically) no Mir server running and the SDL2 lib has been built with X11.
	 * Don't make a full-screen window in this situation.
	 * Also note that to have touch working (if you do have touchscreen on your computer), you need to install libxi-dev
	 * in the desktop kit.
	 *
	 * The phone display has significantly higher resolution than a typical PC screen. This makes the touch images horribly
	 * big. A solution would be to apply a custom scaling or have a different set of images but as this is just a demo
	 * and the desktop build is just for testing I kept is as it is.
	 */
	constexpr int WINDOW_WIDTH = 800;
	constexpr int WINDOW_HEIGHT = 600;

	return SDL2pp::Window("SDLDemo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
#else
	// create a full-screen window
	return SDL2pp::Window("SDLDemo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
#endif
}

SDLDemo::SDLDemo(std::random_device::result_type randomSeed):
	m_window(createWindow()),
	m_renderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
	m_windowX(m_window.GetDrawableWidth()),
	m_windowY(m_window.GetDrawableHeight()),
	m_quit(false),
	m_sceneChanged(false),
	m_randomEngine(randomSeed),
	m_mixer(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 1, 2048),
	m_soundFingerDown(getResourcePath(SOUND_FINGER_DOWN_FILE).native()),
	m_soundFingerUp(getResourcePath(SOUND_FINGER_UP_FILE).native())
{
	for (size_t i = 0; i < COLOR_COUNT; i++)
		m_colorUsage[i] = 0;

	m_mixer.AllocateChannels(16);
}

void SDLDemo::run()
{
	// target frames per second value
	constexpr unsigned fps = 30;

	// use uint32_t to match SDL_GetTicks return type
	constexpr std::uint32_t framePeriod = static_cast<std::uint32_t>(1000.0/fps + 0.5f); // in milliseconds

	bool forceRedraw = true;
	uint32_t lastRedraw;
	SDL_Event event;

	while (!m_quit)
	{
		std::uint32_t now = SDL_GetTicks();
		if (forceRedraw || now >= lastRedraw + framePeriod)
		{
			if (forceRedraw || m_sceneChanged)
			{
				redraw();
				forceRedraw = false;
				m_sceneChanged = false;
			}

			lastRedraw = now;
		}

		now = SDL_GetTicks();
		const std::uint32_t timeSinceRedraw = now - lastRedraw;
		const std::uint32_t maxWait = timeSinceRedraw < framePeriod  ? framePeriod - timeSinceRedraw : 0;

		if (SDL_WaitEventTimeout(&event, maxWait) != 0)
			handleEvent(event);
		// else: timed out
	}
}

void SDLDemo::redraw()
{
	m_renderer.SetDrawColor(0, 0, 0, SDL_ALPHA_OPAQUE);
	m_renderer.Clear();

	for (auto& it : m_fingerImages)
	{
		it.second.fingerImage.draw(m_renderer);
	}
	m_renderer.Present();
}

void SDLDemo::handleEvent(const SDL_Event& event)
{
	switch (event.type)
	{
	case SDL_QUIT:
		m_quit = true;
		break;

	case SDL_FINGERDOWN:
		handleFingerDown(event.tfinger);
		break;

	case SDL_FINGERMOTION:
		handleFingerMotion(event.tfinger);
		break;

	case SDL_FINGERUP:
		handleFingerUp(event.tfinger);
		break;

	case SDL_MOUSEBUTTONDOWN:
		handleMouseButtonDown(event.button);
		break;

	case SDL_MOUSEMOTION:
		handleMouseMotion(event.motion);
		break;

	case SDL_MOUSEBUTTONUP:
		handleMouseButtonUp(event.button);
		break;
	}
}

SDL2pp::Point SDLDemo::getFingerPosition(const SDL_TouchFingerEvent& touchEvent)
{
	const int x = static_cast<int>(std::lround(touchEvent.x * m_windowX));
	const int y = static_cast<int>(std::lround(touchEvent.y * m_windowY));
	return SDL2pp::Point(x, y);
}

void SDLDemo::handleFingerDown(const SDL_TouchFingerEvent& touchEvent)
{
	fingerNew(true, touchEvent.fingerId, getFingerPosition(touchEvent));
}

void SDLDemo::handleFingerMotion(const SDL_TouchFingerEvent& touchEvent)
{
	fingerMove(true, touchEvent.fingerId, getFingerPosition(touchEvent));
}

void SDLDemo::handleFingerUp(const SDL_TouchFingerEvent& touchEvent)
{
	fingerDelete(true, touchEvent.fingerId);
}

void SDLDemo::handleMouseButtonDown(const SDL_MouseButtonEvent& mouseButtonEvent)
{
	// inore mouse emulation by touch as touch events are handled
	if (mouseButtonEvent.which == SDL_TOUCH_MOUSEID)
		return;

	fingerNew(false, mouseButtonEvent.which, SDL2pp::Point(mouseButtonEvent.x, mouseButtonEvent.y));
}

void SDLDemo::handleMouseMotion(const SDL_MouseMotionEvent& mouseMotionEvent)
{
	// inore mouse emulation by touch as touch events are handled
	if (mouseMotionEvent.which == SDL_TOUCH_MOUSEID)
		return;

	if (findFinger(false, mouseMotionEvent.which) != m_fingerImages.end())
		fingerMove(false, mouseMotionEvent.which, SDL2pp::Point(mouseMotionEvent.x, mouseMotionEvent.y));
	// else: assume the button is not down and it's not being tracked
}

void SDLDemo::handleMouseButtonUp(const SDL_MouseButtonEvent& mouseButtonEvent)
{
	// inore mouse emulation by touch as touch events are handled
	if (mouseButtonEvent.which == SDL_TOUCH_MOUSEID)
		return;

	fingerDelete(false, mouseButtonEvent.which);
}

void SDLDemo::fingerNew(bool isTouch, SDL_FingerID fingerId, const SDL2pp::Point& position)
{
	const size_t colorIndex = pickColor();
	m_colorUsage[colorIndex]++;
	Finger finger(colorIndex, FingerImage(m_renderer, position, COLORS[colorIndex]));
	auto r = m_fingerImages.insert(std::make_pair(std::make_pair(isTouch, fingerId), std::move(finger)));
	if (!r.second)
		std::cerr << "newFinger: Finger with ID " << fingerId << " already being tracked!" << std::endl;
	else
		m_sceneChanged = true;

	playSound(m_soundFingerDown);
}

void SDLDemo::fingerMove(bool isTouch, SDL_FingerID fingerId, const SDL2pp::Point& position)
{
	auto it = findFinger(isTouch, fingerId);
	if (it == m_fingerImages.end())
	{
		std::cerr << "fingerMove: Can't find finger with ID " << fingerId << std::endl;
		return;
	}

	it->second.fingerImage.moveTo(position);
	m_sceneChanged = true;
}

void SDLDemo::fingerDelete(bool isTouch, SDL_FingerID fingerId)
{
	playSound(m_soundFingerUp);

	auto it = findFinger(isTouch, fingerId);
	if (it == m_fingerImages.end())
	{
		std::cerr << "fingerDelete: Can't find finger with ID " << fingerId << std::endl;
		return;
	}

	// decrement usage count for the finger's color
	m_colorUsage[it->second.colorIndex]--;

	m_fingerImages.erase(it);
	m_sceneChanged = true;
}

SDLDemo::FingerMap::iterator SDLDemo::findFinger(bool isTouch, SDL_FingerID fingerId)
{
	return m_fingerImages.find(std::make_pair(isTouch, fingerId));
}

size_t SDLDemo::pickColor()
{
	size_t const* const begin = &m_colorUsage[0];
	const size_t* const end = &m_colorUsage[COLOR_COUNT];

	const size_t minUsage = *std::min_element(begin, end);
	const size_t usageCount = std::count(begin, end, minUsage);

	std::uniform_int_distribution<size_t> uniformDistribution(0, usageCount - 1);
	size_t chosenIndex = uniformDistribution(m_randomEngine);

	// find index of the chosenIndex-th entry that is equal to minUsage
	size_t i = 0;
	while (chosenIndex > 0 || m_colorUsage[i] != minUsage)
	{
		if (m_colorUsage[i] == minUsage)
			chosenIndex--;
		i++;
	}

	return i;
}

void SDLDemo::playSound(const SDL2pp::Chunk& chunk)
{
	if (m_mixer.IsChannelPlaying(-1) < m_mixer.GetNumChannels())
		m_mixer.PlayChannel(-1, chunk);
	else
		std::cout << "SDLDemo::playSound: no more free channels, not adding new sound" << std::endl;
}

int main()
{
	SDL2pp::SDL sdl(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL2pp::SDLTTF sdlTTF;
	SDL2pp::SDLMixer sdlMixer;

	SDL_version v;
	SDL_GetVersion(&v);
	std::cout << "Using SDL version: " << static_cast<int>(v.major) << "." << static_cast<int>(v.minor) << "." << static_cast<int>(v.patch) << std::endl;

	initResourcePath();

	std::random_device randomDevice;
	SDLDemo::execute(randomDevice());

	return 0;
}

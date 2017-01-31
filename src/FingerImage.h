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
#ifndef FINGER_IMAGE_H
#define FINGER_IMAGE_H

#include <memory>

#include <SDL.h>
#include <SDL2pp/Rect.hh>
#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Texture.hh>
#include <SDL2pp/Font.hh>

class FingerImage
{
public:
	FingerImage(SDL2pp::Renderer& renderer, const SDL2pp::Point& position, const SDL_Color& color);
	void moveTo(const SDL2pp::Point& position);
	void draw(SDL2pp::Renderer &renderer);

private:
	SDL2pp::Point m_position;
	const SDL_Color m_color;
	std::unique_ptr<SDL2pp::Texture> m_labelTexture;

	static std::unique_ptr<SDL2pp::Texture> contactTexture;
	static SDL2pp::Point contactTextureSize;
	static SDL2pp::Point contactTextureOffset;
	static std::unique_ptr<SDL2pp::Font> font;

	static const char* CONTACT_TEXTURE_FILE;
	static const char* LABEL_FONT_FILE;
	static const int FONT_SIZE;
};

#endif // FINGER_IMAGE_H

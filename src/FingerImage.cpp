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
#include "FingerImage.h"
#include "ResourcePath.h"

#include <SDL2pp/SDLTTF.hh>

#include <sstream>

std::unique_ptr<SDL2pp::Texture> FingerImage::contactTexture;
SDL2pp::Point FingerImage::contactTextureSize;
SDL2pp::Point FingerImage::contactTextureOffset;
std::unique_ptr<SDL2pp::Font> FingerImage::font;

const char* FingerImage::CONTACT_TEXTURE_FILE = "contact.png";
const char* FingerImage::LABEL_FONT_FILE = "FreeSans.otf";
const int FingerImage::FONT_SIZE = 64;

FingerImage::FingerImage(SDL2pp::Renderer& renderer, const SDL2pp::Point& position, const SDL_Color& color):
	m_position(position),
	m_color(color)
{
	if (!contactTexture)
	{
		contactTexture.reset(new SDL2pp::Texture(renderer, getResourcePath(CONTACT_TEXTURE_FILE).native()));

		contactTextureSize = contactTexture->GetSize();

		// assume the "hotspot" is in the middle of the texture
		contactTextureOffset.SetX(-contactTextureSize.GetX()/2);
		contactTextureOffset.SetY(-contactTextureSize.GetY()/2);

		font.reset(new SDL2pp::Font(getResourcePath(LABEL_FONT_FILE).native(), FONT_SIZE));
	}
}

void FingerImage::moveTo(const SDL2pp::Point& position)
{
	if (m_position != position)
	{
		m_position = position;

		// drop the cached label
		m_labelTexture.reset();
	}
}

void FingerImage::draw(SDL2pp::Renderer &renderer)
{
	// image
	SDL2pp::Point texturePosition = m_position + contactTextureOffset;
	contactTexture->SetColorMod(m_color.r, m_color.g, m_color.b);
	renderer.Copy(*contactTexture, SDL2pp::NullOpt, texturePosition);

	// label
	if (!m_labelTexture)
	{
		std::stringstream stream;
		stream << "(" << m_position.GetX() << "," << m_position.GetY() << ")";
		SDL2pp::Surface labelSurface = font->RenderUTF8_Blended(stream.str(), m_color);
		m_labelTexture.reset(new SDL2pp::Texture(renderer, labelSurface));
	}

	const SDL2pp::Point labelTextureSize = m_labelTexture->GetSize();
	const SDL2pp::Point outputSize = renderer.GetOutputSize();

	SDL2pp::Point labelPosition;

	// choose y position of label
	// decide if the label shall be placed below or above the image
	if (texturePosition.GetY() + contactTextureSize.GetY() + labelTextureSize.GetY() <= outputSize.GetY())
	{
		// label fits below the image
		labelPosition.SetY(texturePosition.GetY() + contactTextureSize.GetY());
	}
	else
	{
		// place label above as it doesn't fit below
		labelPosition.SetY(texturePosition.GetY() - labelTextureSize.GetY());
	}

	// place so it's centerer below (or above) contact image
	int preferredX = texturePosition.GetX() + (contactTextureSize.GetX() - labelTextureSize.GetX()) / 2;

	// clamp the value so that the text doesn't leave the screen (well, provided it can even fit...)
	if (preferredX < 0)
		preferredX = 0;
	else if (preferredX + labelTextureSize.GetX() >= outputSize.GetX())
		preferredX = outputSize.GetX() - labelTextureSize.GetX();

	labelPosition.SetX(preferredX);

	renderer.Copy(*m_labelTexture, SDL2pp::NullOpt, labelPosition);
}

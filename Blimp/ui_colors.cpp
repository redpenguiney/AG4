#include "ui_colors.hpp"

std::shared_ptr<Texture> UI_PREFABS::GeoFont(unsigned size) {
	TextureCreateParams menuFontParams({ TextureSource("../fonts/Arial.ttf"), });
	menuFontParams.fontHeight = size;
	menuFontParams.format = Texture::Grayscale_8Bit;
	return std::make_shared<Texture>(menuFontParams, Texture::Texture2D);
}

#pragma once
#include <memory>
#include <texture.hpp>
#include <glm/vec3.hpp>

namespace UI_PREFABS {
	constexpr glm::vec3 MAIN(0.75, 0.4, 0.9);
	constexpr glm::vec3 DARK(0.65, 0.3, 0.8);
	constexpr glm::vec3 HIGHLIGHT(0.95, 0.6, 1);
	constexpr glm::vec3 CONTRAST(1, 1, 1);

	std::shared_ptr<Texture> GeoFont(unsigned size);

	template <unsigned fontSize>
	std::shared_ptr<Texture>& GetMenuFont() {
		static auto font = GeoFont(fontSize);
		return font;
	}
}


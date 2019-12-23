#pragma once
#include <memory>
#include "inline_math.h"

namespace rayt {
	class Texture;
	typedef  std::shared_ptr<Texture> TexturePtr;

	class Texture {
	public:
		virtual vec3 value(float u, float v, const vec3& p) const = 0;
	};

	class ColorTexture : public Texture {
	public:
		ColorTexture(const vec3& c)
			: m_color(c) {
		}

		virtual vec3 value(float u, float v, const vec3& p) const override {
			return m_color;
		}
	private:
		vec3 m_color;
	};
}

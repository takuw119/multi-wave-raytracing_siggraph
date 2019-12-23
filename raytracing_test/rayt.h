//
// Created by Takuto Kishioka on 2019/10/15.
//
#pragma once // To avoid "Redefinition of 'Class'"

#include <iostream>
#include <vector>

#define NUM_THREAD 24

#include "inline_math.h"
#include "Ray.h"

using namespace std;

namespace rayt {
	class Texture;
	typedef  shared_ptr<Texture> TexturePtr;

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

} // namespace rayt
#pragma once
#include <iostream>
#include <array>
#include "Image.h"
#include "Camera.h"
#include "Shape.h"

using namespace std;

namespace rayt {
	class Scene {
	public:
		Scene(int width, int height, int samples)
			: m_image(make_unique<Image>(width, height))
			, m_backColor(0.2f)
			, m_samples(samples) { }

		void build(float r_param, float g_param, float b_param, float refractive_param);

		vec3 color(const rayt::Ray& r, const Shape* world, int depth) const;

		vec3 backgroundSky(const vec3& d) const {
			vec3 v = normalize(d);
			float t = 0.5f * (v.getY() + 1.0f);
			return lerp(t, vec3(1), vec3(0.5f, 0.7f, 1.0f));
		}

		void render(int threadNum, int numThread, Vector3 image[], const Vector3& rgb_param, const float refractive_param);

	private:
		unique_ptr<Camera> m_camera;
		unique_ptr<Image> m_image;
		unique_ptr<Shape> m_world;
		vec3 m_backColor;
		int m_samples;
	};
}

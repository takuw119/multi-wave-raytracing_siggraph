#pragma once
#include <iostream>
#include <array>
#include "inline_math.h"

using namespace std;

namespace rayt {
	class Camera;
	class Image;
	class Shape;
	class Ray;

	class Scene {
	public:
		Scene(int width, int height, int samples);
		~Scene();

		void build(float r_param, float g_param, float b_param, float refractive_param);
		vec3 color(const rayt::Ray& r, const Shape* world, int depth) const;
		void render(int threadNum, int numThread, Vector3 image[], const Vector3& rgb_param, const float refractive_param);

	private:
		unique_ptr<Camera> m_camera;
		unique_ptr<Image> m_image;
		unique_ptr<Shape> m_world;
		vec3 m_backColor;
		int m_samples;
	};
}

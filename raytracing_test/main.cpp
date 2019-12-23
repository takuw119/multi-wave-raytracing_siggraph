//
// Created by Takuto Kishioka on 2019/10/15.
//
#include <iostream>
#include <array>
#include <string>
#include <chrono>
#include <omp.h>

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

		void build(float r_param, float g_param, float b_param, float refractive_param)
		{
			m_backColor = vec3(0);

			// Camera

			vec3 lookfrom(278, 278, -800);
			vec3 lookat(278, 278, 0);
			vec3 vup(0, 1, 0);

			// Zoom
 //           vec3 lookfrom( 180, 18, 60);
 //           vec3 lookat(180, 18, 80);
 //           vec3 vup(0, 1, 0);

			float aspect = float(m_image->width()) / float(m_image->height());
			m_camera = make_unique<Camera>(lookfrom, lookat, vup, 40, aspect);

			// Shapes
			MaterialPtr red = make_shared<Lambertian>(
				make_shared<ColorTexture>(vec3(0.65f, 0.05f, 0.05f)));
			MaterialPtr white = make_shared<Lambertian>(
				make_shared<ColorTexture>(vec3(0.73f, 0.73f, 0.73f)));
			MaterialPtr blue = make_shared<Lambertian>(
				make_shared<ColorTexture>(vec3(0.12f, 0.15f, 0.45f)));
			MaterialPtr light = make_shared<DiffuseLight>(
				make_shared<ColorTexture>(vec3(15.0f * r_param, 15.0f * g_param, 15.0f * b_param)));


			ShapeList* world = new ShapeList();
			world->add(make_shared<FlipNormals>(
				make_shared<Rect>(
					0, 555, 0, 555, 555, Rect::kYZ, blue)));
			world->add(make_shared<Rect>(
				0, 555, 0, 555, 0, Rect::kYZ, red));
			world->add(make_shared<FlipNormals>(
				make_shared<Rect>(
					213, 343, 227, 332, 554, Rect::kXZ, light)));
			world->add(make_shared<FlipNormals>(
				make_shared<Rect>(
					0, 555, 0, 555, 555, Rect::kXZ, white)));
			world->add(make_shared<Rect>(
				0, 555, 0, 555, 0, Rect::kXZ, white));
			world->add(make_shared<FlipNormals>(
				make_shared<Rect>(
					0, 555, 0, 555, 555, Rect::kXY, white)));

			/*world->add(make_shared<Triangle>(
				40, 0, 380, 330, Triangle::kXY, red));
			*/

			world->add(make_shared<Sphere>(
					vec3(200, 125, 200), 125,
					make_shared<Dielectric>(refractive_param)));

					/*world->add(
							make_shared<Translate>(
									make_shared<Rotate>(
													make_shared<Box>(vec3(130, 0, 65), vec3(295,165,230), make_shared<Dielectric>(2.01f)),
											vec3(0,1,0),-90),
									vec3(130, 0, 65)));*/
									//world->add(make_shared<Box>(vec3(130, 0, 65), vec3(295, 165, 230), make_shared<Dielectric>(2.01f)));

			m_world.reset(world);
		}

		vec3 color(const rayt::Ray& r, const Shape* world, int depth) const {
			HitRec hrec;
			if (world->hit(r, 0.001, FLT_MAX, hrec)) {
				vec3 emitted = hrec.mat->emitted(r, hrec);
				ScatterRec srec;
				if (depth < MAX_DEPTH && hrec.mat->scatter(r, hrec, srec)) {
					return emitted + mulPerElem(srec.albedo, color(srec.ray, world, depth + 1));
				}
				else {
					return emitted;
				}
			}
			return background(r.direction());
		}

		vec3 background(const vec3& d) const {
			return m_backColor;
		}

		vec3 backgroundSky(const vec3& d) const {
			vec3 v = normalize(d);
			float t = 0.5f * (v.getY() + 1.0f);
			return lerp(t, vec3(1), vec3(0.5f, 0.7f, 1.0f));
		}

		void render(int threadNum, int numThread, Vector3 image[], const Vector3& rgb_param, const float refractive_param)
		{
			build(rgb_param.getX(), rgb_param.getY(), rgb_param.getZ(), refractive_param);

			int nx = m_image->width();
			int ny = m_image->height();

			auto begin = ny / numThread * threadNum;
			auto end = begin + ny / numThread;
			for (int j = begin; j < end; ++j) {
				for (int i = 0; i < nx; ++i) {
					vec3 c(0);
					for (int s = 0; s < m_samples; ++s) {
						float u = (float(i) + drand48()) / float(nx);
						float v = (float(j) + drand48()) / float(ny);
						Ray r = m_camera->getRay(u, v);
						c += color(r, m_world.get(), 0);
					}
					c /= m_samples;
					image[nx * (ny - j - 1) + i] = c;
				}
			}
		}

	private:
		unique_ptr<Camera> m_camera;
		unique_ptr<Image> m_image;
		unique_ptr<Shape> m_world;
		vec3 m_backColor;
		int m_samples;
	};

}// namespace rayt

constexpr int nx = 408;
constexpr int ny = 408;
constexpr int ns = 2000;
constexpr int NUM_THREAD = 24;

constexpr int PIXEL_COUNT = nx * ny;

int nxs[NUM_THREAD];
int nys[NUM_THREAD];
int nss[NUM_THREAD];

//const array<Vector3, 1> rgb_params = { Vector3{1.0, 1.0, 1.0} };

const array<Vector3, 7> rgb_params = {
	Vector3{0.271110203, 0.002383286468, 0.0003824933941},
	Vector3{0.2826850333, 0.1646411679, 0.02534749569},
	Vector3{0.2661447962, 0.2170198516, 0.03028760031},
	Vector3{0.04673523311, 0.3052432179 , 0.116319581},
	Vector3{0.00544537513, 0.1864855434, 0.2827487676},
	Vector3{0.05021897786, 0.08084457278, 0.3138387983},
	Vector3{0.07766038141, 0.04338235985, 0.2310752638}
};


const array<float, 7> refractive_params = {
	1.98,
	1.99,
	2.01,
	2.04,
	2.06,
	2.09
};

void render(Vector3 pixels[], const Vector3& rgb_param, const float refractive_params)

{
	for (int i = 0; i < NUM_THREAD; i++) {
		nxs[i] = nx;
		nys[i] = ny;
		nss[i] = ns;
	}

	auto begin = std::chrono::high_resolution_clock::now();

#pragma omp parallel num_threads(NUM_THREAD)
	{
		int threadNum = omp_get_thread_num();
		unique_ptr<rayt::Scene> scene(make_unique<rayt::Scene>(nxs[threadNum], nys[threadNum], nss[threadNum]));

		scene->render(threadNum, NUM_THREAD, pixels, rgb_param, refractive_params);

	}

	auto end = std::chrono::high_resolution_clock::now();

	const double time = std::chrono::duration_cast<std::chrono::duration<double>>(end - begin).count();
	std::cout << "time " << time << "[s]" << std::endl;
}

void save(const string& file_path, Vector3 pixels[])
{
	rayt::Image image(nx, ny);
	rayt::Image::rgb char8Pixels[PIXEL_COUNT];
	for (int i = 0; i < PIXEL_COUNT; ++i)
	{
		char8Pixels[i] = image.getWrite(pixels[i]);
	}
	stbi_write_bmp(file_path.c_str(), nx, ny, sizeof(rayt::Image::rgb), char8Pixels);
}

int main()
{
	auto sum_pixels = make_unique<Vector3[]>(PIXEL_COUNT);
	for (int i = 0; i < PIXEL_COUNT; ++i)
	{
		sum_pixels[i] = { 0,0,0 };
	}

	for (int i = 0; i < rgb_params.size(); i++)
	{
		auto ray_pixels = make_unique<Vector3[]>(PIXEL_COUNT);
		render(ray_pixels.get(), rgb_params[i], refractive_params[i]);

		string file_path = "ray_" + to_string(i) + ".bmp";
		save(file_path, ray_pixels.get());

		for (int i = 0; i < PIXEL_COUNT; ++i)
		{
			sum_pixels[i] += ray_pixels[i];
		}
	}

	save("ray_sum.bmp", sum_pixels.get());

	return 0;
}
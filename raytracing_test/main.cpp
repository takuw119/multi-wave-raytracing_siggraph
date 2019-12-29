//
// Created by Takuto Kishioka on 2019/10/15.
//
#include <iostream>
#include <string>
#include <chrono>
#include <omp.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include "Scene.h"
#include "Image.h"

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

//const array<float, 1> refractive_params = { 2.01 };
const array<float, 7> refractive_params = {
	1.98,
	1.99,
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
	auto rgb8uPixels = make_unique<rayt::Image::rgb[]>(PIXEL_COUNT);
	for (int i = 0; i < PIXEL_COUNT; ++i)
	{
		rgb8uPixels[i] = image.getWrite(pixels[i]);
	}
	stbi_write_bmp(file_path.c_str(), nx, ny, sizeof(rayt::Image::rgb), rgb8uPixels.get());
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
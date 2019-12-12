//
// Created by Takuto Kishioka on 2019/10/15.
//
#pragma once // To avoid "Redefinition of 'Class'"

#include <memory> // To use "unique_ptr"
#include <iostream>

#include <vector>

#define NUM_THREAD 24

#include <float.h>  // FLT_MIN, FLT_MAX
#define PI 3.14159265359f
#define PI2 6.28318530718f
#define RECIP_PI 0.31830988618f
#define RECIP_PI2 0.15915494f
#define LOG2 1.442695f
#define EPSILON 1e-6f
#define GAMMA_FACTOR 2.2f

#define MAX_DEPTH 50

#include <random>
inline double drand48() {
	return float(((double)(rand()) / (RAND_MAX))); /* RAND_MAX = 32767 */
}

inline float pow2(float x) { return x * x; }
inline float pow3(float x) { return x * x * x; }
inline float pow4(float x) { return x * x * x * x; }
inline float pow5(float x) { return x * x * x * x * x; }
inline float clamp(float x, float a, float b) { return x < a ? a : x > b ? b : x; }
inline float saturate(float x) { return x < 0.f ? 0.f : x > 1.f ? 1.f : x; }
inline float recip(float x) { return 1.f / x; }
inline float mix(float a, float b, float t) { return a * (1.f - t) + b * t; /* return a + (b-a) * t; */ }
inline float step(float edge, float x) { return (x < edge) ? 0.f : 1.f; }
inline float smoothstep(float a, float b, float t) { if (a >= b) return 0.f; float x = saturate((t - a) / (b - a)); return x * x * (3.f - 2.f * t); }
inline float radians(float deg) { return (deg / 180.f) * PI; }
inline float degrees(float rad) { return (rad / PI) * 180.f; }

// To calculate Vector
#include "vectormath/include/vectormath/scalar/cpp/vectormath_aos.h"
using namespace Vectormath::Aos;
typedef Vector3 vec3;
typedef Vector3 col3;

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

using namespace std;


namespace rayt {


	//--------------------------------------------------------------------------------

	inline vec3 linear_to_gamma(const vec3& v, float gammaFactor) {
		float recipGammaFactor = recip(gammaFactor);
		return vec3(
			powf(v.getX(), recipGammaFactor),
			powf(v.getY(), recipGammaFactor),
			powf(v.getZ(), recipGammaFactor));
	}

	inline vec3 gamma_to_linear(const vec3& v, float gammaFactor) {
		return vec3(
			powf(v.getX(), gammaFactor),
			powf(v.getY(), gammaFactor),
			powf(v.getZ(), gammaFactor));
	}

	inline vec3 reflect(const vec3& v, const vec3& n) {
		return v - 2.f * dot(v, n) * n;
	}

	inline bool refract(const vec3& v, const vec3& n, float ni_over_nt, vec3& refracted) {
		vec3 uv = normalize(v);
		float dt = dot(uv, n);
		float D = 1.f - pow2(ni_over_nt) * (1.f - pow2(dt));
		if (D > 0.f) {
			refracted = -ni_over_nt * (uv - n * dt) - n * sqrt(D);
			return true;
		}
		else {
			return false;
		}
	}

	inline float schlick(float cosine, float ri) {
		float r0 = pow2((1.f - ri) / (1.f + ri));
		return r0 + (1.f - r0) * pow5(1.f - cosine);
	}


	//--------------------------------------------------------------------------------

	inline vec3 random_vector() {
		return vec3(drand48(), drand48(), drand48());
	}

	inline vec3 random_in_uint_sphere() {
		vec3 p;
		do {
			p = 2.f * random_vector() - vec3(1.f);
		} while (lengthSqr(p) >= 1.f);
		return p;
	}

	//--------------------------------------------------------------------------------

	class ImageFilter {
	public:
		virtual vec3 filter(const vec3& c) const = 0;
	};

	class GammaFilter : public ImageFilter {
	public:
		GammaFilter(float factor) : m_factor(factor) {}
		virtual vec3 filter(const vec3& c) const override {
			return linear_to_gamma(c, m_factor);
		}
	private:
		float m_factor;
	};

	//--------------------------------------------------------------------------------

	class TonemapFilter : public ImageFilter {
	public:
		TonemapFilter() {}
		virtual vec3 filter(const vec3& c) const override {
			return minPerElem(maxPerElem(c, Vector3(0.f)), Vector3(1.f));
		}
	};

	//--------------------------------------------------------------------------------

	class Image {
	public:
		struct rgb {
			unsigned char r;
			unsigned char g;
			unsigned char b;
			void operator+=(const rgb& other) {
				r += other.r;
				g += other.g;
				b += other.b;
			}
		};

		Image() : m_pixels(nullptr) { }
		Image(int w, int h) {
			m_width = w;
			m_height = h;
			m_pixels.reset(new rgb[m_width * m_height]);
			m_filters.push_back(make_unique<GammaFilter>(GAMMA_FACTOR));
			m_filters.push_back(make_unique<TonemapFilter>());
		}

		int width() const { return m_width; }
		int height() const { return m_height; }
		void* pixels() const { return m_pixels.get(); }

		void write(int x, int y, float r, float g, float b) {
			vec3 c(r, g, b);
			for (auto& f : m_filters) {
				c = f->filter(c);
			}
			int index = m_width * y + x;
			m_pixels[index].r = static_cast<unsigned char>(c.getX() * (255.99f));
			m_pixels[index].g = static_cast<unsigned char>(c.getY() * (255.99f));
			m_pixels[index].b = static_cast<unsigned char>(c.getZ() * (255.99f));
		}
		rgb getWrite(int x, int y, float r, float g, float b) {
			vec3 c(r, g, b);
			for (auto& f : m_filters) {
				c = f->filter(c);
			}
			rgb pixel;
			pixel.r = static_cast<unsigned char>(c.getX() * (255.99f));
			pixel.g = static_cast<unsigned char>(c.getY() * (255.99f));
			pixel.b = static_cast<unsigned char>(c.getZ() * (255.99f));
			return pixel;
		}
	private:
		int m_width;
		int m_height;
		unique_ptr<rgb[]> m_pixels;
		vector< unique_ptr<ImageFilter> > m_filters;
	};

	//-------------------------------------------------------------------------

	class Ray {
	public:
		Ray() {}
		Ray(const vec3& o, const vec3& dir)
			: m_origin(o)
			, m_direction(dir) { }

		const vec3& origin() const { return m_origin; }
		const vec3& direction() const { return m_direction; }
		vec3 at(float t) const { return m_origin + t * m_direction; }

	private:
		vec3 m_origin; // Start Point
		vec3 m_direction; // Direction (Denormalized)
	};

	//--------------------------------------------------------------------------

	class Camera {
	public:
		Camera() {}
		Camera(const vec3& u, const vec3& v, const vec3& w) {
			m_origin = vec3(0);
			m_uvw[0] = u;
			m_uvw[1] = v;
			m_uvw[2] = w;
		}
		Camera(const vec3& lookfrom, const vec3& lookat, const vec3& vup, float vfov, float aspect) {
			vec3 u, v, w;
			float halfH = tanf(radians(vfov) / 2.0f);
			float halfW = aspect * halfH;
			m_origin = lookfrom;
			w = normalize(lookfrom - lookat);
			u = normalize(cross(vup, w));
			v = cross(w, u);
			m_uvw[2] = m_origin - halfW * u - halfH * v - w;
			m_uvw[0] = 2.0f * halfW * u;
			m_uvw[1] = 2.0f * halfH * v;
		}

		Ray getRay(float u, float v) const {
			return Ray(m_origin, m_uvw[2] + m_uvw[0] * u + m_uvw[1] * v - m_origin);
		}

	private:
		vec3 m_origin; // Position
		vec3 m_uvw[3]; // Orthogonal Basis Vectors
	};

	//-----------------------------------------------------------------

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



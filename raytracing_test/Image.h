#pragma once
#include <memory> // To use "unique_ptr"
#include "inline_math.h"

using namespace std;

namespace rayt {
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
		rgb getWrite(const vec3& color) {
			vec3 c = color;
			for (auto& f : m_filters) {
				c = f->filter(c);
			}
			c *= 255.9;

			rgb pixel;
			pixel.r = static_cast<unsigned char>(c.getX());
			pixel.g = static_cast<unsigned char>(c.getY());
			pixel.b = static_cast<unsigned char>(c.getZ());
			return pixel;
		}
	private:
		int m_width;
		int m_height;
		unique_ptr<rgb[]> m_pixels;
		vector< unique_ptr<ImageFilter> > m_filters;
	};
}

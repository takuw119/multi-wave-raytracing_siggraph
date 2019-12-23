#pragma once
#include "Ray.h"
#include "inline_math.h"
#include "Texture.h"

namespace rayt {
	class Shape;
	class Material;
	typedef std::shared_ptr<Shape> ShapePtr;
	typedef std::shared_ptr<Material> MaterialPtr;

	//----------------------------------------------------------------------------


	//----------------------------------------------------------------------------

	class HitRec {
	public:
		float t;
		float u;
		float v;
		vec3 p;
		vec3 n;
		MaterialPtr mat;
	};

	class ScatterRec {
	public:
		Ray ray;
		vec3 albedo;
	};

	class Material {
	public:
		virtual bool scatter(const Ray& r, const HitRec& hrec, ScatterRec& srec) const = 0;
		virtual vec3 emitted(const Ray& r, const HitRec& hrec) const { return vec3(0); }
	};

	//----------------------------------------------------------------------------

	class Lambertian : public Material {
	public:
		Lambertian(const TexturePtr& a)
			: m_albedo(a) {
		}
		virtual bool scatter(const Ray& r, const HitRec& hrec, ScatterRec& srec) const override {
			vec3 target = hrec.p + hrec.n + random_in_uint_sphere();
			srec.ray = Ray(hrec.p, target - hrec.p);
			srec.albedo = m_albedo->value(hrec.u, hrec.v, hrec.p);
			return true;
		}
	private:
		TexturePtr m_albedo;
	};

	//----------------------------------------------------------------------------

	class Metal : public Material {
	public:
		Metal(const TexturePtr& a, float fuzz)
			: m_albedo(a)
			, m_fuzz(fuzz) {
		}

		virtual bool scatter(const Ray& r, const HitRec& hrec, ScatterRec& srec) const override {
			vec3 reflected = reflect(normalize(r.direction()), hrec.n);
			reflected += m_fuzz * random_in_uint_sphere();
			srec.ray = Ray(hrec.p, reflected);
			srec.albedo = m_albedo->value(hrec.u, hrec.v, hrec.p);
			return dot(srec.ray.direction(), hrec.n) > 0;
		}

	private:
		TexturePtr m_albedo;
		float m_fuzz;
	};

	class Dielectric : public Material {
	public:
		Dielectric(float ri)
			: m_ri(ri) {

		}
		virtual bool scatter(const Ray& r, const HitRec& hrec, ScatterRec& srec) const override {

			vec3 outward_normal;
			vec3 reflected = reflect(r.direction(), hrec.n);
			float ni_over_nt;
			float reflect_prob;
			float cosine;
			if (dot(r.direction(), hrec.n) > 0) {
				outward_normal = -hrec.n;
				ni_over_nt = m_ri;
				cosine = m_ri * dot(r.direction(), hrec.n) / length(r.direction());
			}
			else {
				outward_normal = hrec.n;
				ni_over_nt = recip(m_ri);
				cosine = -dot(r.direction(), hrec.n) / length(r.direction());
			}
			srec.albedo = vec3(1);

			vec3 refracted;
			if (refract(-r.direction(), outward_normal, ni_over_nt, refracted)) {
				reflect_prob = schlick(cosine, m_ri);
			}
			else {
				reflect_prob = 1;
			}

			if (drand48() < reflect_prob) {
				srec.ray = Ray(hrec.p, reflected);
			}
			else {
				srec.ray = Ray(hrec.p, refracted);
			}

			return true;
		}

	private:
		float m_ri;
	};

	class DiffuseLight : public Material {
	public:
		DiffuseLight(const TexturePtr& emit)
			: m_emit(emit) {
		}

		virtual bool scatter(const Ray& r, const HitRec& hrec, ScatterRec& srec) const override {
			return false;
		}

		virtual vec3 emitted(const Ray& r, const HitRec& hrec) const override {
			return m_emit->value(hrec.u, hrec.v, hrec.p);
		}

	private:
		TexturePtr m_emit;
	};

}

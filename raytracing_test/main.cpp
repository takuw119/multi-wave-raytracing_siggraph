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
#include "Texture.h"

using namespace std;

namespace rayt {
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	class Shape;
	class Material;
	typedef shared_ptr<Shape> ShapePtr;
	typedef shared_ptr<Material> MaterialPtr;

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

	//------------------------------------------------------------------------------

	class Shape {
	public:
		virtual bool hit(const Ray& r, float t0, float t1, HitRec& hrec) const = 0;
	};

	//----------------------------------------------------------------------------

	class ShapeList : public Shape {
	public:
		ShapeList() {}

		void add(const ShapePtr& shape) {
			m_list.push_back(shape);
		}

		virtual bool hit(const Ray& r, float t0, float t1, HitRec& hrec) const override {
			HitRec temp_rec;
			bool hit_anything = false;
			float closest_so_far = t1;
			for (auto& p : m_list) {
				if (p->hit(r, t0, closest_so_far, temp_rec)) {
					hit_anything = true;
					closest_so_far = temp_rec.t;
					hrec = temp_rec;
				}
			}
			return hit_anything;
		}

	private:
		vector<ShapePtr> m_list;
	};

	//------------------------------------------------------------------------------

	class Sphere : public Shape {
	public:
		Sphere() {}
		Sphere(const vec3& c, float r, const MaterialPtr& mat)
			: m_center(c)
			, m_radius(r)
			, m_material(mat) { }

		virtual bool hit(const Ray& r, float t0, float t1, HitRec& hrec) const override {
			vec3 oc = r.origin() - m_center;
			float a = dot(r.direction(), r.direction());
			float b = 2.0f * dot(oc, r.direction());
			float c = dot(oc, oc) - pow2(m_radius);
			float D = b * b - 4 * a * c;
			if (D > 0) {
				float root = sqrtf(D);
				float temp = (-b - root) / (2.0f * a);
				if (temp < t1 && temp > t0) {
					hrec.t = temp;
					hrec.p = r.at(hrec.t);
					hrec.n = (hrec.p - m_center) / m_radius;
					hrec.mat = m_material;
					return true;
				}
				temp = (-b + root) / (2.0f * a);
				if (temp < t1 && temp > t0) {
					hrec.t = temp;
					hrec.p = r.at(hrec.t);
					hrec.n = (hrec.p - m_center) / m_radius;
					hrec.mat = m_material;
					return true;
				}
			}
			return false;
		}

	private:
		vec3 m_center;
		float m_radius;
		MaterialPtr m_material;
	};

	//----------------------------------------------------------------------------

	class Rect : public Shape {
	public:
		enum AxisType {
			kXY = 0,
			kXZ,
			kYZ
		};
		Rect() {}
		Rect(float x0, float x1, float y0, float y1, float k, AxisType axis, const MaterialPtr& m)
			: m_x0(x0)
			, m_x1(x1)
			, m_y0(y0)
			, m_y1(y1)
			, m_k(k)
			, m_axis(axis)
			, m_material(m) {
		}

		virtual bool hit(const Ray& r, float t0, float t1, HitRec& hrec) const override {
			int xi, yi, zi;
			vec3 axis;
			switch (m_axis) {
			case kXY: xi = 0; yi = 1; zi = 2; axis = vec3::zAxis(); break;
			case kXZ: xi = 0; yi = 2; zi = 1; axis = vec3::yAxis(); break;
			case kYZ: xi = 1; yi = 2; zi = 0; axis = vec3::xAxis(); break;
			}

			float t = (m_k - r.origin()[zi]) / r.direction()[zi];
			if (t < t0 || t > t1) {
				return false;
			}

			float x = r.origin()[xi] + t * r.direction()[xi];
			float y = r.origin()[yi] + t * r.direction()[yi];
			if (x < m_x0 || x > m_x1 || y < m_y0 || y > m_y1) {
				return false;
			}

			hrec.u = (x - m_x0) / (m_x1 - m_x0);
			hrec.v = (y - m_y0) / (m_y1 - m_y0);
			hrec.t = t;
			hrec.mat = m_material;
			hrec.p = r.at(t);
			hrec.n = axis;
			return true;
		}
	private:
		float m_x0, m_x1, m_y0, m_y1, m_k;
		AxisType m_axis;
		MaterialPtr m_material;
	};

	//----------------------------------------------------------------------------

	class FlipNormals : public Shape {
	public:
		FlipNormals(const ShapePtr& shape)
			: m_shape(shape) {
		}

		virtual bool hit(const Ray& r, float t0, float t1, HitRec& hrec) const override {
			if (m_shape->hit(r, t0, t1, hrec)) {
				hrec.n = -hrec.n;
				return true;
			}
			else {
				return false;
			}
		}

	private:
		ShapePtr m_shape;
	};

	//----------------------------------------------------------------------------

	class Box : public Shape {
	public:
		Box() {}
		Box(const vec3& p0, const vec3& p1, const MaterialPtr& m)
			: m_p0(p0)
			, m_p1(p1)
			, m_list(make_unique<ShapeList>()) {

			ShapeList* l = new ShapeList();
			l->add(make_shared<Rect>(
				p0.getX(), p1.getX(), p0.getY(), p1.getY(), p1.getZ(), Rect::kXY, m));
			l->add(make_shared<FlipNormals>(make_shared<Rect>(
				p0.getX(), p1.getX(), p0.getY(), p1.getY(), p0.getZ(), Rect::kXY, m)));
			l->add(make_shared<Rect>(
				p0.getX(), p1.getX(), p0.getZ(), p1.getZ(), p1.getY(), Rect::kXZ, m));
			l->add(make_shared<FlipNormals>(make_shared<Rect>(
				p0.getX(), p1.getX(), p0.getZ(), p1.getZ(), p0.getY(), Rect::kXZ, m)));
			l->add(make_shared<Rect>(
				p0.getY(), p1.getY(), p0.getZ(), p1.getZ(), p1.getX(), Rect::kYZ, m));
			l->add(make_shared<FlipNormals>(make_shared<Rect>(
				p0.getY(), p1.getY(), p0.getZ(), p1.getZ(), p0.getX(), Rect::kYZ, m)));
			m_list.reset(l);
		}

		virtual bool hit(const Ray& r, float t0, float t1, HitRec& hrec) const override {
			return m_list->hit(r, t0, t1, hrec);
		}

	private:
		vec3 m_p0, m_p1;
		unique_ptr<ShapeList> m_list;
	};

	//----------------------------------------------------------------------------

	class Translate : public Shape {
	public:
		Translate(const ShapePtr& sp, const vec3& displacement)
			: m_shape(sp)
			, m_offset(displacement) {
		}

		virtual bool hit(const Ray& r, float t0, float t1, HitRec& hrec) const override {
			Ray move_r(r.origin() - m_offset, r.direction());
			if (m_shape->hit(move_r, t0, t1, hrec)) {
				hrec.p += m_offset;
				return true;
			}
			else {
				return false;
			}
		}

	private:
		ShapePtr m_shape;
		vec3 m_offset;
	};

	class Rotate : public Shape {
	public:
		Rotate(const ShapePtr& sp, const vec3& axis, float angle)
			: m_shape(sp)
			, m_quat(Quat::rotation(radians(angle), axis)) {
		}

		virtual bool hit(const Ray& r, float t0, float t1, HitRec& hrec) const override {
			Quat revq = conj(m_quat);
			vec3 origin = rotate(revq, r.origin());
			vec3 direction = rotate(revq, r.direction());
			Ray rot_r(origin, direction);
			if (m_shape->hit(rot_r, t0, t1, hrec)) {
				hrec.p = rotate(m_quat, hrec.p);
				hrec.n = rotate(m_quat, hrec.n);
				return true;
			}
			else {
				return false;
			}
		}

	private:
		ShapePtr m_shape;
		Quat m_quat;
	};

	//----------------------------------------------------------------------------

	class Triangle : public Shape {
	public:
		enum AxisType {
			kXY = 0,
			kXZ,
			kYZ
		};
		Triangle() {}
		Triangle(float x0, float y0, float l, float k, AxisType axis, const MaterialPtr& m)
			: m_x0(x0)
			, m_y0(y0)
			, m_l(l)
			, m_k(k)
			, m_axis(axis)
			, m_material(m) {
		}

		virtual bool hit(const Ray& r, float t0, float t1, HitRec& hrec) const override {
			int xi, yi, zi;
			vec3 axis;
			switch (m_axis) {
			case kXY: xi = 0; yi = 1; zi = 2; axis = vec3::zAxis(); break;
			case kXZ: xi = 0; yi = 2; zi = 1; axis = vec3::yAxis(); break;
			case kYZ: xi = 1; yi = 2; zi = 0; axis = vec3::xAxis(); break;
			}

			float t = (m_k - r.origin()[zi]) / r.direction()[zi];
			if (t < t0 || t > t1) {
				return false;
			}

			float x = r.origin()[xi] + t * r.direction()[xi];
			float y = r.origin()[yi] + t * r.direction()[yi];

			if ((y - m_y0) > sqrt(3)* (x - m_x0) || (y - m_y0) > -sqrt(3) * (x - m_x0) + 2 * (m_l + m_y0) || y < m_y0) {
				return false;
			}

			hrec.u = (x - m_x0) / m_l;
			hrec.v = y - m_y0;
			hrec.t = t;
			hrec.mat = m_material;
			hrec.p = r.at(t);
			hrec.n = axis;
			return true;
		}
	private:
		float m_x0, m_y0, m_l, m_k;
		AxisType m_axis;
		MaterialPtr m_material;

	};


	//----------------------------------------------------------------------------

	/*class Prism : public Shape {
	public:
		Prism(){}
		Prism(const vec3& p0, float l,float d, const MaterialPtr& m)
			: m_p0(p0)
			, m_l(l)
			, m_d(d)
			, m_list(make_unique<ShapeList>()) {
			ShapeList* 1 = new ShapeList();
			1->add(make_shared<Triangle>(
				p0.getX(), p0.getY(), m_l, p0.getZ(), Triangle::kXY, m));
			1->add(make_shared<Triangle>(
				p0.getX(), p0.getY(), m_l, p0.getZ(), Triangle::kXY, m));
			1->add(make_shared<Rect>(
				p0.getX(), p0.getY(), m_l, p0.getZ(), Rect::kXY, m));
			1->add(make_shared<Rect>(
				p0.getX(), p0.getY(), m_l, p0.getZ(), Rect::kXY, m));
			1->add(make_shared<Rect>(
				p0.getX(), p0.getY(), m_l, p0.getZ(), Rect::kXY, m));
			m_list.reset(1);
		}
		virtual bool hit(const Ray& r, float t0, float t1, HitRec& hrec) const override {
			return m_list->hit(r, t0, t1, hrec);
		}
	private:
		vec3 m_p0;
		float m_l, m_d;
		unique_ptr<ShapeList> m_list;

	};*/

	//----------------------------------------------------------------------------


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
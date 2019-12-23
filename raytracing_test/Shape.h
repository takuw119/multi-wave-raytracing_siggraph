#pragma once
#include "Material.h"

namespace rayt {
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
		std::vector<ShapePtr> m_list;
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

			if ((y - m_y0) > sqrt(3)* (x - m_x0) || (y - m_y0) > -sqrt(3) * (x - m_x0) + 2 * (sqrt(3) * m_l / 2 + m_y0) || y < m_y0) {
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

	class Prism : public Shape {
	public:
		Prism() {}
		Prism(const vec3& p0, float l, float d, const MaterialPtr& m)
			: m_p0(p0)
			, m_l(l)
			, m_d(d)
			, m_list(make_unique<ShapeList>()) {
			ShapeList* list = new ShapeList();
			list->add(make_shared<Triangle>(
				p0.getX(), p0.getY(), m_l, p0.getZ(), Triangle::kXY, m));
			list->add(make_shared<FlipNormals>(make_shared<Triangle>(
				p0.getX(), p0.getY(), m_l, p0.getZ() + m_d, Triangle::kXY, m)));
			list->add(make_shared<FlipNormals>(make_shared<Rect>(
				p0.getX(), p0.getX() + m_l, p0.getZ(), p0.getZ() + m_d, p0.getY(), Rect::kXZ, m)));
			list->add(make_shared<Rotate>(make_shared<Rect>(
				p0.getY(), p0.getY() + m_l, p0.getZ(), p0.getZ() + m_d, p0.getX(), Rect::kYZ, m), vec3(0, 0, 1), -30));
			list->add(make_shared<FlipNormals>(make_shared<Rotate>(make_shared<Rect>(
				p0.getY(), p0.getY() + m_l, p0.getZ(), p0.getZ() + m_d, p0.getX() + m_l, Rect::kYZ, m), vec3(0, 0, 1), 30)));
			m_list.reset(list);
		}
		virtual bool hit(const Ray& r, float t0, float t1, HitRec& hrec) const override {
			return m_list->hit(r, t0, t1, hrec);
		}
	private:
		vec3 m_p0;
		float m_l, m_d;
		unique_ptr<ShapeList> m_list;
	};
}

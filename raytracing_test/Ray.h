#pragma once
#include "inline_math.h"

namespace rayt {
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
}

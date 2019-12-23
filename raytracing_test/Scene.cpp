#include "Scene.h"
using namespace rayt;

void Scene::build(float r_param, float g_param, float b_param, float refractive_param)
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

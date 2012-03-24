#pragma once

#include <Demo.h>

class PathTraceDemo : public Demo
{
	public:
		static Demo* createFunc( const Device* deviceData ) { return new PathTraceDemo(deviceData); }

		PathTraceDemo(const Device* device);
		~PathTraceDemo();

		void init();

		void step(float dt);

		void renderPost();

		void render();


		struct Ray
		{
			Ray(){}
			Ray(const float4& o, const float4& d) : m_orig(o), m_dir(d) {}

			float4 m_orig;
			float4 m_dir;
		};

		struct IntersectOut
		{
			float m_dist;
			float4 m_normal;

			bool hasHit() const { return (m_dist!=-1); }
		};

		struct Sphere
		{
			Sphere() { m_color = make_float4(1,1,1,0); m_emission = make_float4(0.0f); }
			void intersect(const Ray& ray, IntersectOut& out);

			float4 m_pos;
			float4 m_color;
			float4 m_emission;
		};

		enum
		{
			PATH_TRACE = 1,

			NUM_SPHERES = 5,
		};


		int intersect(const Ray& ray, IntersectOut& out);

	public:
		float4* m_frameBuffer;
		Buffer<float4>* m_buffer;




		struct Camera
		{
			float4 m_from;
			float4 m_to;
			float4 m_up;
			float m_fov;
		};

		Camera m_camera;
		Sphere* m_sphere;
		float m_time;
};

#pragma once

#include <Adl/Adl.h>
#include <AdlPrimitives/Math/Math.h>
#include <Demos/Bp/BroadphaseBase.h>


namespace adl
{
class RigidBodyBase
{
	public:

		_MEM_CLASSALIGN16
		struct Body
		{
			_MEM_ALIGNED_ALLOCATOR16;

			float4 m_pos;
			Quaternion m_quat;
			float4 m_linVel;
			float4 m_angVel;

			u32 m_shapeIdx;
			float m_invMass;
			float m_restituitionCoeff;
			float m_frictionCoeff;
			
		};

		struct Shape
		{
/*			u16 m_shapeType;
			u16 m_shapeIdx;
			float m_restituitionCoeff;
			float m_frictionCoeff;
			int m_padding;
*/
			Matrix3x3 m_invInertia;
			Matrix3x3 m_initInvInertia;
		};
/*
		struct BodyData
		{
			Buffer<Body> m_bodies;
		};

		struct ShapeData
		{
			Buffer<Shape> m_shapes;
		};

		struct AabbData
		{
			Buffer<AabbUint> m_aabbs;
		};
*/
		static
		__inline
		Buffer<Body>* allocateBodyData( const Device* device, int capacity );

		static
		__inline
		Buffer<Shape>* allocateShapeData( const Device* device, int capacity );

		static
		__inline
		Buffer<AabbUint>* allocateAabbData( const Device* device, int capacity );

		static
		__inline
		void deallocateBodyData( Buffer<Body>* data );

		static
		__inline
		void deallocateShapeData( Buffer<Shape>* data );

		static
		__inline
		void deallocateAabbData( Buffer<AabbUint>* data );
};

template<DeviceType TYPE>
class RigidBody : public RigidBodyBase
{
	public:
		typedef Launcher::BufferInfo BufferInfo;

		struct Data
		{
			const Device* m_device;

			Kernel* m_integrateKernel;
			Kernel* m_updateInertiaKernel;
			Kernel* m_applyGravityKernel;
		};

		//	functions
		static
		Data* allocate( const Device* device );

		static
		void deallocate( Data* data );

		static
		void integrate( Data* data, Buffer<Body>* bodyData, int n, float dt );

		static
		void updateInertia( Data* data, Buffer<Body>* bodyData, Buffer<Shape>* shapeData, int n );

		static
		void applyGravity( Data* data, Buffer<Body>* bodyData, int n, const float4& gravity, float dt );

		static
		void calculateAabb( Data* data, Buffer<Body>* bodyData, Buffer<Aabb>* shapeAabbs, 
			Buffer<AabbUint>* bodyAabbs, int n, const Aabb& space, const float4& margin );
};

//#include <AdlPhysics/RigidBody/RigidBody.inl>
//#include <AdlPhysics/RigidBody/RigidBodyHost.inl>
};


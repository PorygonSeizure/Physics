#ifndef _SPHERE_H_
#define _SPHERE_H_

#include "RigidBody.h"

namespace Physics
{
class Sphere : public RigidBody
{
public:
	Sphere() : RigidBody(ShapeType::SPHERE), m_radius(1.f) {}
	Sphere(float radius) : RigidBody(ShapeType::SPHERE), m_radius(radius) {}
	Sphere(glm::vec3 position, float radius) : RigidBody(ShapeType::SPHERE), m_radius(radius) { m_position = position; }
	virtual ~Sphere() {}

	virtual void Update(glm::vec3 gravity, float deltaTime) { RigidBody::Update(gravity, deltaTime); }
	virtual void MakeGizmo(glm::vec4 color);
	virtual void ApplyForce(glm::vec3 force) { RigidBody::ApplyForce(force); }

	virtual inline void SetRadius(float radius) { m_radius = radius; }

	virtual inline const float& GetRadius() const { return m_radius; }

protected:
	float m_radius;
};
}

#endif
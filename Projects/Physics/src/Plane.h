#ifndef _PLANE_H_
#define _PLANE_H_

#include "PhysicsObject.h"
#include <glm/glm.hpp>

namespace Physics
{
class Plane : public PhysicsObject
{
public:
	Plane() : PhysicsObject(ShapeType::PLANE), m_distanceToOrigin(0.f), m_normal(glm::vec3(0.f, 1.f, 0.f)) {}
	virtual ~Plane();

	virtual void Update(glm::vec3 gravity, float deltaTime) {}
	virtual void Debug() {}
	virtual void MakeGizmo(glm::vec4 color);

	glm::vec3 m_normal;
	float m_distanceToOrigin;

protected:
	
};
}

#endif
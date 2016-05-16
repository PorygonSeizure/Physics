#ifndef _PLANE_H_
#define _PLANE_H_

#include "PhysicsObject.h"
#include <glm/glm.hpp>

namespace Physics
{
class Plane : public PhysicsObject
{
public:
	Plane(float distance, glm::vec3 normal) : PhysicsObject(ShapeType::PLANE), m_distanceToOrigin(distance), m_normal(normal) {}
	virtual ~Plane();

	virtual void Update(glm::vec3 gravity, float deltaTime) {}
	virtual void Debug() {}
	virtual void MakeGizmo(glm::vec4 color);
	//virtual void SetPosition(glm::vec3 position);

	//virtual glm::vec3 GetPosition();

protected:
	glm::vec3 m_normal;
	float m_distanceToOrigin;
	float m_k;
};
}

#endif
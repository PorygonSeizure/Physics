#ifndef _SPRING_H_
#define _SPRING_H_

#include "PhysicsObject.h"

namespace Physics
{
class RigidBody;

class Spring : public PhysicsObject
{
public:
	Spring(RigidBody* body1, RigidBody* body2, float stiffness, float damping);
	virtual ~Spring() {}

	virtual void Update(glm::vec3 gravity, float deltaTime);
	virtual void MakeGizmo(glm::vec4 color);

protected:
	RigidBody* m_rigidbodys[2];
	float m_damping;
	float m_restLength;
	float m_stiffness;
};
}

#endif
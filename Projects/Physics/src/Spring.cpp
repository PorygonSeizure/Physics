#include "Spring.h"
#include "RigidBody.h"
#include <Gizmos.h>

using namespace Physics;

Spring::Spring(RigidBody* body1, RigidBody* body2, float stiffness, float damping) : PhysicsObject(JOINT)
{
	m_rigidbodys[0] = body1;
	m_rigidbodys[1] = body2;
	m_stiffness = stiffness;
	m_damping = damping;
	m_restLength = glm::length(m_rigidbodys[0]->GetPosition() - m_rigidbodys[1]->GetPosition());
}

void Spring::Update(glm::vec3 gravity, float deltaTime)
{
	glm::vec3 springVec = m_rigidbodys[0]->GetPosition() - m_rigidbodys[1]->GetPosition();
	float currentLength = glm::length(springVec);

	if (currentLength != m_restLength)
	{
		glm::vec3 force;

		if (currentLength != 0.f)
			force += -(springVec / currentLength) * (currentLength - m_restLength) * m_stiffness;

		force += -(m_rigidbodys[0]->GetVelocity() - m_rigidbodys[1]->GetVelocity()) * m_damping;

		m_rigidbodys[0]->ApplyForce(force);
		m_rigidbodys[1]->ApplyForce(-force);
	}
}

void Spring::MakeGizmo(glm::vec4 color) { Gizmos::AddLine(m_rigidbodys[0]->GetPosition(), m_rigidbodys[1]->GetPosition(), color); }
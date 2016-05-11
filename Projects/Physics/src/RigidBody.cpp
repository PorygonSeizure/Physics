#include "RigidBody.h"

using namespace Physics;

void RigidBody::Update(glm::vec3 gravity, float deltaTime)
{
	//apply some fake friction
	ApplyForce(-m_velocity * m_dampening);

	m_velocity += m_acceleration * deltaTime;
	m_position += m_velocity * deltaTime;

	m_acceleration = gravity;
}

void RigidBody::ApplyForce(glm::vec3 force)
{
	m_acceleration += force / m_mass;
}
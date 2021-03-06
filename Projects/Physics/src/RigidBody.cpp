#include "RigidBody.h"

using namespace Physics;
using glm::vec3;

void RigidBody::Update(vec3 gravity, float deltaTime)
{
	//apply some fake friction
	ApplyForce(-m_velocity * m_dampening);

	m_velocity += m_acceleration * deltaTime;
	m_position += m_velocity * deltaTime;

	m_acceleration = gravity;
}

void RigidBody::ApplyForceToActor(RigidBody* otherBody, vec3 force)
{
	ApplyForce(-force);
	otherBody->ApplyForce(force);
}
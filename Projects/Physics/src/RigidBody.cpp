#include "RigidBody.h"

using namespace Physics;
using glm::vec3;

RigidBody::RigidBody(ShapeType shapeID) : PhysicsObject(shapeID), m_dampening(1.f), m_bouciness(1.f) { SetMass(1.f); }

void RigidBody::Update(vec3 gravity, float deltaTime)
{
	//apply some fake friction
	ApplyForce(-m_velocity * m_dampening);

	m_velocity += m_acceleration * deltaTime;
	m_position += m_velocity * deltaTime;

	m_acceleration = gravity;
}

void RigidBody::ApplyForce(vec3 force)
{
	m_acceleration += force / m_mass;
}

void RigidBody::ApplyForceToActor(RigidBody* otherBody, vec3 force)
{
	ApplyForce(-force);
	otherBody->ApplyForce(force);
}
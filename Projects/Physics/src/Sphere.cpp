#include "Sphere.h"

#include <Gizmos.h>

using namespace Physics;

Sphere::~Sphere()
{

}

void Sphere::Update(glm::vec3 gravity, float deltaTime)
{
	RigidBody::Update(gravity, deltaTime);
}

void Sphere::Debug()
{

}

void Sphere::MakeGizmo(glm::vec4 color)
{
	Gizmos::AddSphere(m_position, m_radius, 8, 8, color);
}

void Sphere::ApplyForce(glm::vec3 force)
{
	RigidBody::ApplyForce(force);
}
#include "Box.h"

#include <Gizmos.h>

using namespace Physics;
using glm::vec3;

Box::~Box()
{

}

void Box::Update(glm::vec3 gravity, float deltaTime)
{
	RigidBody::Update(gravity, deltaTime);
}

void Box::Debug()
{

}

void Box::MakeGizmo(glm::vec4 color)
{
	Gizmos::AddAABBFilled(m_position, vec3(m_length, m_height, m_width), color);
}

void Box::ApplyForce(vec3 force)
{
	RigidBody::ApplyForce(force);
}

void Box::SetLength(float length)
{
	m_length = length;
}

void Box::SetHeight(float height)
{
	m_height = height;
}

void Box::SetWidth(float width)
{
	m_width = width;
}

void Box::CreateAABB()
{
	m_minVert = m_position - vec3(m_length / 2.f, 0, 0) - vec3(0, 0, m_width / 2.f) - vec3(0, m_height / 2.f, 0);
	m_maxVert = m_position + vec3(m_length / 2.f, 0, 0) - vec3(0, 0, m_width / 2.f) + vec3(0, m_height / 2.f, 0);
}
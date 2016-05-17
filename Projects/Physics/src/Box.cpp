#include "Box.h"

#include <Gizmos.h>

using namespace Physics;
using glm::vec3;

void Box::Update(vec3 gravity, float deltaTime)
{
	RigidBody::Update(gravity, deltaTime);
	SetOBB();
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

void Box::SetOBB()
{
	m_verts[0] = m_position - (m_length / 2.f) - (m_height / 2.f) - (m_width / 2.f);
	m_verts[1] = m_position - (m_length / 2.f) - (m_height / 2.f) + (m_width / 2.f);
	m_verts[2] = m_position - (m_length / 2.f) + (m_height / 2.f) - (m_width / 2.f);
	m_verts[3] = m_position - (m_length / 2.f) + (m_height / 2.f) + (m_width / 2.f);
	m_verts[4] = m_position + (m_length / 2.f) - (m_height / 2.f) - (m_width / 2.f);
	m_verts[5] = m_position + (m_length / 2.f) - (m_height / 2.f) + (m_width / 2.f);
	m_verts[6] = m_position + (m_length / 2.f) + (m_height / 2.f) - (m_width / 2.f);
	m_verts[7] = m_position + (m_length / 2.f) + (m_height / 2.f) + (m_width / 2.f);

	m_axis[0] = m_verts[4] - m_verts[0];
	m_axis[1] = m_verts[2] - m_verts[0];
	m_axis[2] = m_verts[1] - m_verts[0];

	for (int i = 0; i < 3; i++)
	{
		float temp = glm::length(m_axis[i]);
		temp *= temp;
		m_axis[i] /= temp;
		m_origin[i] = glm::dot(m_verts[0], m_axis[i]);
	}
}

vec3 Box::ClosestPoint(const vec3& point)
{
	vec3 closestPoint = m_position;
	vec3 d = point - m_position;

	for (int i = 0; i < 3; i++)
	{
		float dist = glm::dot(d, m_axis[i]);

		if (dist > m_origin[i])
			dist = m_origin[i];
		else if (dist > -m_origin[i])
			dist = -m_origin[i];

		closestPoint += dist * m_axis[i];
	}
	return closestPoint;
}
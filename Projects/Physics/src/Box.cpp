#include "Box.h"

#include <Gizmos.h>

using namespace Physics;
using glm::vec3;

Box::Box(glm::vec3 position, float xOffset, float yOffset, float zOffset) : RigidBody(ShapeType::BOX), m_xOffset(xOffset), m_yOffset(yOffset), m_zOffset(zOffset)
{
	m_position = position;
	SetOBB();
}

void Box::Update(vec3 gravity, float deltaTime)
{
	RigidBody::Update(gravity, deltaTime);
	SetOBB();
}

void Box::MakeGizmo(glm::vec4 color) { Gizmos::AddAABBFilled(m_position, vec3(m_xOffset, m_yOffset, m_zOffset), color); }

void Box::SetOBB()
{
	m_verts[0] = vec3(m_position.x - m_xOffset, m_position.y - m_yOffset, m_position.z - m_zOffset);
	m_verts[1] = vec3(m_position.x - m_xOffset, m_position.y - m_yOffset, m_position.z + m_zOffset);
	m_verts[2] = vec3(m_position.x + m_xOffset, m_position.y - m_yOffset, m_position.z + m_zOffset);
	m_verts[3] = vec3(m_position.x + m_xOffset, m_position.y - m_yOffset, m_position.z - m_zOffset);
	m_verts[4] = vec3(m_position.x - m_xOffset, m_position.y + m_yOffset, m_position.z - m_zOffset);
	m_verts[5] = vec3(m_position.x - m_xOffset, m_position.y + m_yOffset, m_position.z + m_zOffset);
	m_verts[6] = vec3(m_position.x + m_xOffset, m_position.y + m_yOffset, m_position.z + m_zOffset);
	m_verts[7] = vec3(m_position.x + m_xOffset, m_position.y + m_yOffset, m_position.z - m_zOffset);

	m_axis[0] = m_verts[3] - m_verts[0];
	m_axis[1] = m_verts[4] - m_verts[0];
	m_axis[2] = m_verts[1] - m_verts[0];

	for (int i = 0; i < 3; i++)
	{
		m_axis[i] /= glm::dot(m_axis[i], m_axis[i]);
		m_origin[i] = glm::dot(m_verts[0], m_axis[i]);
	}
}

vec3 Box::ClosestPoint(const vec3& point)
{
	SetOBB();

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
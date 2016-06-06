#include "Plane.h"

#include <Gizmos.h>

using namespace Physics;
using glm::vec3;

void Plane::MakeGizmo(glm::vec4 color)
{
	float lineSegmentLength = 300;
	vec3 centrePoint = m_normal * m_distanceToOrigin;
	vec3 parallel = vec3(m_normal.y, -m_normal.x, m_normal.z); //easy to rotate normal through 90degrees around z
	vec3 start = centrePoint + (parallel * lineSegmentLength);
	vec3 end = centrePoint - (parallel * lineSegmentLength);
	Gizmos::AddLine(start, end, color);
}

//void Plane::SetPosition(glm::vec3 position)
//{
//	float temp1 = (m_normal.x * position.x) + (m_normal.y * position.y) + (m_normal.z * position.z);
//	float temp2 = (m_normal.x * m_normal.x) + (m_normal.y * m_normal.y) + (m_normal.z * m_normal.z);
//	m_k = temp1 / temp2;
//	m_distanceToOrigin = glm::abs(temp1) / sqrt(temp2);
//}
//
//vec3 Plane::GetPosition()
//{
//	vec3 pos;
//	pos.x = -(m_k * m_normal.x);
//	pos.y = -(m_k * m_normal.y);
//	pos.z = -(m_k * m_normal.z);
//}
#include "Plane.h"

#include <Gizmos.h>

using namespace Physics;

Plane::~Plane()
{

}

void Plane::MakeGizmo(glm::vec4 color)
{
	float lineSegmentLength = 300;
	glm::vec3 centrePoint = m_normal * m_distanceToOrigin;
	glm::vec3 parallel = glm::vec3(m_normal.y, -m_normal.x, m_normal.z); //easy to rotate normal through 90degrees around z
	glm::vec3 start = centrePoint + (parallel * lineSegmentLength);
	glm::vec3 end = centrePoint - (parallel * lineSegmentLength);
	Gizmos::AddLine(start, end, color);
}
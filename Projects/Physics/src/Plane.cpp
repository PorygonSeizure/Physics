#include "Plane.h"

#include <Gizmos.h>

using namespace Physics;
using glm::vec3;

Plane::~Plane()
{

}

void Plane::MakeGizmo(glm::vec4 color)
{
	float lineSegmentLength = 300;
	vec3 centrePoint = m_normal * m_distanceToOrigin;
	vec3 parallel = vec3(m_normal.y, -m_normal.x, m_normal.z); //easy to rotate normal through 90degrees around z
	vec3 start = centrePoint + (parallel * lineSegmentLength);
	vec3 end = centrePoint - (parallel * lineSegmentLength);
	Gizmos::AddLine(start, end, color);
}
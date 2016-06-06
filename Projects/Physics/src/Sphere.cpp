#include "Sphere.h"

#include <Gizmos.h>

using namespace Physics;

void Sphere::MakeGizmo(glm::vec4 color) { Gizmos::AddSphere(m_position, m_radius, 8, 8, color); }
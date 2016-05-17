#ifndef _BOX_H_
#define _BOX_H_

#include "RigidBody.h"

namespace Physics
{
class Box : public RigidBody
{
public:
	Box() : RigidBody(ShapeType::BOX), m_xOffset(1.f), m_yOffset(1.f), m_zOffset(1.f) { SetOBB(); }
	Box(float value) : RigidBody(ShapeType::BOX), m_xOffset(value), m_yOffset(value), m_zOffset(value) { SetOBB(); }
	Box(float xOffset, float yOffset, float zOffset) : RigidBody(ShapeType::BOX), m_xOffset(xOffset), m_yOffset(yOffset), m_zOffset(zOffset) { SetOBB(); }
	Box(glm::vec3 position, float xOffset, float yOffset, float zOffset) : RigidBody(ShapeType::BOX), m_xOffset(xOffset), m_yOffset(yOffset), m_zOffset(zOffset) { m_position = position; SetOBB(); }
	virtual ~Box() {}

	virtual void Update(glm::vec3 gravity, float deltaTime);
	virtual void Debug();
	virtual void MakeGizmo(glm::vec4 color);
	virtual void ApplyForce(glm::vec3 force);

	inline void SetXOffset(float xOffset) { m_xOffset = xOffset; }
	inline void SetYOffset(float yOffset) { m_yOffset = yOffset; }
	inline void SetZOffset(float zOffset) { m_zOffset = zOffset; }
	void SetOBB();

	glm::vec3 ClosestPoint(const glm::vec3& point);
	inline glm::vec3 GetMinVert() { return m_verts[0]; }
	inline glm::vec3 GetMaxVert() { return m_verts[7]; }

protected:
	float m_xOffset;
	float m_yOffset;
	float m_zOffset;

	glm::vec3 m_verts[8];
	glm::vec3 m_axis[3];
	float m_origin[3];

	//glm::mat3 xRotation;
	//glm::mat3 yRotation;
	//glm::mat3 zRotation;
};
}

#endif
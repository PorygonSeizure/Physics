#ifndef _BOX_H_
#define _BOX_H_

#include "RigidBody.h"

namespace Physics
{
class Box : public RigidBody
{
public:
	Box();
	Box(float value);
	Box(float xOffset, float yOffset, float zOffset);
	Box(glm::vec3 position, float xOffset, float yOffset, float zOffset);
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
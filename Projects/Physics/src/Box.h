#ifndef _BOX_H_
#define _BOX_H_

#include "RigidBody.h"

namespace Physics
{
class Box : public RigidBody
{
public:
	Box() : RigidBody(ShapeType::BOX), m_length(1.f), m_height(1.f), m_width(1.f) { SetOBB(); }
	Box(float value) : RigidBody(ShapeType::BOX), m_length(value), m_height(value), m_width(value) { SetOBB(); }
	Box(float length, float height, float width) : RigidBody(ShapeType::BOX), m_length(length), m_height(height), m_width(width) { SetOBB(); }
	Box(glm::vec3 position, float length, float height, float width) : RigidBody(ShapeType::BOX), m_length(length), m_height(height), m_width(width) { m_position = position; SetOBB(); }
	virtual ~Box() {}

	virtual void Update(glm::vec3 gravity, float deltaTime);
	virtual void Debug();
	virtual void MakeGizmo(glm::vec4 color);
	virtual void ApplyForce(glm::vec3 force);

	inline void SetLength(float length) { m_length = length; }
	inline void SetHeight(float height) { m_height = height; }
	inline void SetWidth(float width) { m_width = width; }
	void SetOBB();

	glm::vec3 ClosestPoint(const glm::vec3& point);
	inline glm::vec3 GetMinVert() { return m_verts[0]; }
	inline glm::vec3 GetMaxVert() { return m_verts[7]; }

protected:
	float m_length;
	float m_height;
	float m_width;

	glm::vec3 m_verts[8];
	glm::vec3 m_axis[3];
	float m_origin[3];

	//glm::mat3 xRotation;
	//glm::mat3 yRotation;
	//glm::mat3 zRotation;
};
}

#endif
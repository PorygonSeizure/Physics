#ifndef _BOX_H_
#define _BOX_H_

#include "RigidBody.h"

namespace Physics
{
class Box : public RigidBody
{
public:
	Box() : RigidBody(ShapeType::BOX), m_length(1.f), m_height(1.f), m_width(1.f) { CreateAABB(); }
	Box(float length, float height, float width) : RigidBody(ShapeType::BOX), m_length(length), m_height(height), m_width(width) { CreateAABB(); }
	inline Box(glm::vec3 position, float length, float height, float width) : RigidBody(ShapeType::BOX), m_length(length), m_height(height), m_width(width) { m_position = position; CreateAABB(); }
	virtual ~Box();

	virtual void Update(float deltaTime);
	virtual void Debug();
	virtual void MakeGizmo(glm::vec4 color);
	virtual void ApplyForce(glm::vec3 force);

	void SetLength(float length);
	void SetHeight(float height);
	void SetWidth(float width);
	void CreateAABB();

	glm::vec3 GetMinVert() { return m_minVert; }
	glm::vec3 GetMaxVert() { return m_maxVert; }

protected:
	float m_length;
	float m_height;
	float m_width;

	glm::vec3 m_minVert;
	glm::vec3 m_maxVert;
};
}

#endif
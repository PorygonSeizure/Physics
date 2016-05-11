#ifndef _PHYSICS_OBJECT_H_
#define _PHYSICS_OBJECT_H_

#include <glm/glm.hpp>

namespace Physics
{
enum ShapeType
{
	PLANE = 0,
	SPHERE = 1,
	BOX = 2
};

class PhysicsObject
{
public:
	PhysicsObject(ShapeType shapeID) : m_shapeID(shapeID) {}
	virtual ~PhysicsObject() {}

	virtual void Update(glm::vec3 gravity, float deltaTime) = 0;
	virtual void Debug() = 0;
	virtual void MakeGizmo(glm::vec4 color) = 0;
	virtual void ResetPosition() {}

	ShapeType GetShapeID() { return m_shapeID; }
	virtual inline void SetPosition(glm::vec3 position) { m_position = position; }
	virtual inline const glm::vec3& GetPosition() const { return m_position; }

protected:
	ShapeType m_shapeID;

	glm::vec3 m_position;
};
}

#endif
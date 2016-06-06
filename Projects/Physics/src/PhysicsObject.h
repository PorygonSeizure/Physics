#ifndef _PHYSICS_OBJECT_H_
#define _PHYSICS_OBJECT_H_

#include <glm/glm.hpp>

namespace Physics
{
enum ShapeType
{
	PLANE = 0,
	SPHERE = 1,
	BOX = 2,
	JOINT = 3
};

class PhysicsObject
{
public:
	PhysicsObject(ShapeType shapeID) : m_shapeID(shapeID) {}
	virtual ~PhysicsObject() {}

	virtual void Update(glm::vec3 gravity, float deltaTime) = 0;
	virtual void MakeGizmo(glm::vec4 color) = 0;
	virtual void ResetPosition() {}
	//virtual void SetPosition(glm::vec3 position) = 0;

	//virtual glm::vec3 GetPosition() = 0;

	inline ShapeType GetShapeID() { return m_shapeID; }

protected:
	ShapeType m_shapeID;
};
}

#endif
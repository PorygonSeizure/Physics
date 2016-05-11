#ifndef _RIGID_BODY_H_
#define _RIGID_BODY_H_

#include "PhysicsObject.h"
#include <glm/glm.hpp>

namespace Physics
{
class RigidBody : public PhysicsObject
{
public:
	inline RigidBody(ShapeType shapeID) : PhysicsObject(shapeID) { SetMass(1.f); SetDampening(1.f); }
	virtual ~RigidBody() {}

	virtual void Update(glm::vec3 gravity, float deltaTime);
	virtual void Debug() = 0;
	virtual void MakeGizmo(glm::vec4 color) = 0;
	void ApplyForce(glm::vec3 force);
	void ApplyForceToActor(RigidBody* otherBody, glm::vec3 force);

	virtual inline void SetVelocity(glm::vec3 velocity) { m_velocity = velocity; }
	virtual inline void SetAcceleration(glm::vec3 acceleration) { m_acceleration = acceleration; }
	virtual inline void SetMass(float mass) { m_mass = mass; m_invMass = 1.f / mass; }
	virtual inline void SetDampening(float dampening) { m_dampening = glm::max(0.f, glm::min(1.f, dampening)); }

	virtual inline const glm::vec3& GetVelocity() const { return m_velocity; }
	virtual inline const glm::vec3& GetAcceleration() const { return m_acceleration; }

	virtual inline const float& GetMass() const { return m_mass; }
	virtual inline const float& GetInverseMass() const { return m_invMass; }
	virtual inline const float& GetDampening() const { return m_dampening; }

protected:
	glm::vec3 m_velocity;
	glm::vec3 m_acceleration;

	float m_mass;
	float m_invMass;
	float m_bouciness;
	float m_dampening;
};
}

#endif
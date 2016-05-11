#ifndef _PHYSICS_APP_H_
#define _PHYSICS_APP_H_

#include "BaseApp.h"

//only needed for the camera picking
#include <glm/vec3.hpp>

class Camera;

namespace Physics
{
	class RigidBody;
	class PhysicsScene;
	class PhysicsRenderer;
}

class PhysicsApp : public BaseApp
{
public:
	PhysicsApp() : m_camera(nullptr) {}
	virtual ~PhysicsApp() {};

	virtual bool Startup();
	virtual void Shutdown();

	virtual bool Update(float deltaTime);
	virtual void Draw();

	void DrawGizmosGrid();

protected:
	Camera* m_camera;

	//this is an example position for camera picking
	glm::vec3 m_pickPosition;

	Physics::PhysicsRenderer* m_physicsRenderer;
	Physics::PhysicsScene* m_physicsScene;
	Physics::RigidBody* m_objects;
};

#endif
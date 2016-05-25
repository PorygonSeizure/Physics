#ifndef _PHYSX_APP_H_
#define _PHYSX_APP_H_

#include "BaseApp.h"

//only needed for the camera picking
#include <glm/vec3.hpp>
#include <PxPhysicsAPI.h>
#include <vector>

class FlyCamera;

namespace Physics
{
//class PhysicsObject;
//class RigidBody;
//class PhysicsScene;
//class PhysicsRenderer;
}

class PhysXApp : public BaseApp
{
public:
	PhysXApp() : m_camera(nullptr) {}
	virtual ~PhysXApp() {};

	virtual bool Startup();
	virtual void Shutdown();

	virtual bool Update(float deltaTime);
	virtual void Draw();

	void SetupScene();
	void DrawGizmosGrid();
	void AddWidget(physx::PxShape* shape, physx::PxRigidActor* actor);
	void AddSphere(physx::PxShape* shape, physx::PxRigidActor* actor);
	void AddPlane(physx::PxShape* shape, physx::PxRigidActor* actor);
	void AddCapsule(physx::PxShape* shape, physx::PxRigidActor* actor);
	void AddBox(physx::PxShape* shape, physx::PxRigidActor* actor);

protected:
	FlyCamera* m_camera;

	//this is an example position for camera picking
	glm::vec3 m_pickPosition;

	//Physics::PhysicsRenderer* m_physicsRenderer;
	//Physics::PhysicsScene* m_physicsScene;
	bool m_wasLeftMousePressed;

	physx::PxDefaultAllocator m_allocator;
	physx::PxDefaultErrorCallback m_errorCallback;

	physx::PxFoundation* m_foundation = NULL;
	physx::PxPhysics* m_physics = NULL;
	physx::PxDefaultCpuDispatcher* m_dispatcher = NULL;
	physx::PxScene* m_scene = NULL;
	physx::PxMaterial* m_material = NULL;
	physx::PxVisualDebuggerConnection* m_connection = NULL;
	//physx::PxRigidDynamic* m_actor = NULL;

	std::vector<physx::PxRigidActor*> m_physXActors;
	std::vector<physx::PxArticulation*> m_ragdolls;
};

#endif
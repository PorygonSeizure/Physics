#ifndef _PHYSX_APP_H_
#define _PHYSX_APP_H_

#include "BaseApp.h"

//only needed for the camera picking
#include <glm/vec3.hpp>
#include <PxPhysicsAPI.h>
#include <vector>
//#include "tinyOBJLoader.h"

class FlyCamera;
class Mesh;
class Shader;
class ParticleEmitter;
class ParticleFluidEmitter;
//class FBXFile;

struct OpenGLInfo
{
	unsigned int m_VAO;
	unsigned int m_VBO;
	unsigned int m_IBO;
	unsigned int m_indexCount;
};

class PhysXApp : public BaseApp
{
public:
	PhysXApp() : m_camera(nullptr) {}
	virtual ~PhysXApp() {};

	virtual bool Startup();
	virtual bool Update(float deltaTime);

	virtual void Draw();
	virtual void Shutdown();

	void SetupScene();
	void AddWidget(physx::PxShape* shape, physx::PxRigidActor* actor);
	void AddSphere(physx::PxShape* shape, physx::PxRigidActor* actor);
	void AddPlane(physx::PxShape* shape, physx::PxRigidActor* actor);
	void AddCapsule(physx::PxShape* shape, physx::PxRigidActor* actor);
	void AddBox(physx::PxShape* shape, physx::PxRigidActor* actor);
	void CreateShader();
	void CreateOpenGLBuffers();
	void CleanupOpenGLBuffers();
	void AttachedRigidBodyConvex(float density, physx::PxMaterial* physicsMaterial, physx::PxRigidActor* actor);

protected:
	FlyCamera* m_camera;

	//this is an example position for camera picking
	glm::vec3 m_pickPosition;

	bool m_wasLeftMousePressed;

	physx::PxDefaultAllocator m_allocator;
	physx::PxDefaultErrorCallback m_errorCallback;

	physx::PxFoundation* m_foundation = NULL;
	physx::PxPhysics* m_physics = NULL;
	physx::PxDefaultCpuDispatcher* m_dispatcher = NULL;
	physx::PxScene* m_scene = NULL;
	physx::PxMaterial* m_material = NULL;
	physx::PxVisualDebuggerConnection* m_connection = NULL;
	physx::PxSimulationEventCallback* m_collisionEventCallback = NULL;
	//physx::PxRigidDynamic* m_actor = NULL;
	physx::PxCooking* m_cooker = NULL;

	std::vector<physx::PxRigidActor*> m_physXActors;
	std::vector<physx::PxArticulation*> m_ragdolls;

	Mesh* m_mesh;
	Shader* m_shaders;
	//ParticleEmitter* m_particleEmitter;
	ParticleFluidEmitter* m_particleFluidEmitter;
	//FBXFile* m_FBX;
};

#endif
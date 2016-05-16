#ifndef _PHYSICS_APP_H_
#define _PHYSICS_APP_H_

#include <BaseApp.h>
#include <PxPhysicsAPI.h>
#include <PxScene.h>
#include <glm/glm.hpp>

class FlyCamera;
class Renderer;

class PhysicsApp : public BaseApp
{
public:
	PhysicsApp() {}
	virtual ~PhysicsApp() {}

	virtual bool Startup();
	virtual bool Update(float deltaTime);

	virtual void Shutdown();
	virtual void Draw();

	void RenderGizmos(physx::PxScene* physicsScene);
	
protected:
    int m_windowWidth;
    int m_windowHeight;

	FlyCamera* m_camera;
	glm::vec3 m_pickPosition;

	Renderer* m_renderer;
};

#endif //_APPLICATION_H_
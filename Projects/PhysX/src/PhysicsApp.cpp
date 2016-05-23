#include "PhysicsApp.h"
#include "gl_core_4_4.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "Camera.h"
#include "Gizmos.h"

using glm::vec3;
using glm::vec4;
using namespace physx;

bool PhysicsApp::Startup()
{
	//create a basic window
	CreateGLFWWindow("AIE OpenGL Application", 1280, 720);

	//start the gizmo system that can draw basic shapes
	Gizmos::Create();

	//create a camera
	m_camera = new FlyCamera(glm::pi<float>() * 0.25f, 16.f / 9.f, 0.1f, 1000.f);
	m_camera->SetLookAtFrom(vec3(10, 10, 10), vec3(0));



	foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
	PxProfileZoneManager* profileZoneManager = &PxProfileZoneManager::createProfileZoneManager(foundation);
	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), true, profileZoneManager);

	if (physics->getPvdConnectionManager())
	{
		physics->getVisualDebugger()->setVisualizeConstraints(true);
		physics->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_CONTACTS, true);
		physics->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES, true);
		connection = PxVisualDebuggerExt::createConnection(physics->getPvdConnectionManager(), "127.0.0.1", 5425, 10);
	}



	m_wasLeftMousePressed = false;
	m_pickPosition = vec3(0);
	//m_physicsRenderer = new Physics::PhysicsRenderer();

	SetupScene();

	return true;
}

void PhysicsApp::Shutdown()
{
	scene->release();
	dispatcher->release();
	PxProfileZoneManager* profileZoneManager = physics->getProfileZoneManager();
	if (connection != NULL)
		connection->release();
	physics->release();
	profileZoneManager->release();
	foundation->release();



	//delete m_physicsRenderer;
	//delete m_physicsScene;

	//delete our camera and cleanup gizmos
	delete m_camera;
	Gizmos::Destroy();

	//destroy our window properly
	DestroyGLFWWindow();
}

void PhysicsApp::SetupScene()
{
	//m_physicsScene = new Physics::PhysicsScene();
	//
	//auto obj1 = m_physicsScene->CreatePhysicsObject<Physics::Sphere>(0.25f);
	//obj1->SetPosition(vec3(-5.f, 5.f, -5.f));
	//m_physicsRenderer->GetRenderInfo(obj1)->color = vec4(rand() % 255 / 255.f, rand() % 255 / 255.f, rand() % 255 / 255.f, 1.f);

	//auto obj2 = m_physicsScene->CreatePhysicsObject<Physics::Box>(0.25f);
	//obj2->SetPosition(vec3(5.f, 5.f, 5.f));
	//m_physicsRenderer->GetRenderInfo(obj2)->color = vec4(rand() % 255 / 255.f, rand() % 255 / 255.f, rand() % 255 / 255.f, 1.f);

	//auto obj3 = m_physicsScene->CreatePhysicsObject<Physics::Spring>(obj1, obj2, 0.5f, 0.9999f);
	//m_physicsRenderer->GetRenderInfo(obj3)->color = vec4(rand() % 255 / 255.f, rand() % 255 / 255.f, rand() % 255 / 255.f, 1.f);



	PxSceneDesc sceneDesc(physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.f, -9.81f, 0.f);
	dispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = dispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	scene = physics->createScene(sceneDesc);

	material = physics->createMaterial(0.5f, 0.5f, 0.6f);

	PxRigidStatic* groundPlane = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
	scene->addActor(*groundPlane);

	PxShape* shape = physics->createShape(PxSphereGeometry(0.25f), *material);
	actor = physics->createRigidDynamic(PxTransform(-5.f, 5.f, -5.f));
	actor->attachShape(*shape);
	scene->addActor(*actor);
}

bool PhysicsApp::Update(float deltaTime)
{
	//close the application if the window closes
	if (glfwWindowShouldClose(m_window) || glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		return false;

	//update the camera's movement
	m_camera->Update(deltaTime);

	vec4 translationVec = m_camera->GetTransform()[3];
	vec4 forwardVec = m_camera->GetTransform()[2];
	vec4 rightVec = m_camera->GetTransform()[0];

	bool leftMousePressed = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS);



	scene->simulate(deltaTime);
	scene->fetchResults(true);
	PxTransform transform = actor->getGlobalPose();

	printf("%0.2f\t%0.2f\t%0.2f\n", transform.p.x, transform.p.y, transform.p.z);



	//if (leftMousePressed && !m_wasLeftMousePressed)
	//{
	//	auto obj = m_physicsScene->CreatePhysicsObject<Physics::Sphere>(0.25f);
	//	obj->SetPosition(vec3(translationVec));
	//	obj->SetVelocity(vec3(-forwardVec * 10.f));
	//	m_physicsRenderer->GetRenderInfo(obj)->color = vec4(rand() % 255 / 255.f, rand() % 255 / 255.f, rand() % 255 / 255.f, 1.f);
	//	m_wasLeftMousePressed = true;
	//}
	//else if (!leftMousePressed && m_wasLeftMousePressed)
	//	m_wasLeftMousePressed = false;
	//
	//m_physicsScene->Simulate(vec3(0, -0.98, 0), deltaTime);

	//return true, else the application closes
	return true;
}

void PhysicsApp::Draw()
{
	//clear the gizmos out for this frame
	Gizmos::Clear();

	DrawGizmosGrid();

	//m_physicsRenderer->Render(m_physicsScene);

	PxActor* listOfActors[1];
	scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC, listOfActors, 1);
	PxShape* listOfShapes[1];
	PxRigidDynamic* temp = listOfActors[0]->isRigidDynamic();
	temp->getShapes(listOfShapes, 1);
	PxTransform transform = temp->getGlobalPose();
	vec3 position(transform.p.x, transform.p.y, transform.p.z);
	PxGeometryHolder geometryHolder = listOfShapes[0]->getGeometry();
	//PxGeometryType::Enum geometryEnum = geometryHolder.getType();
	vec4 color = vec4(rand() % 255 / 255.f, rand() % 255 / 255.f, rand() % 255 / 255.f, 1.f);
	switch (geometryHolder.getType())
	{
	case PxGeometryType::eSPHERE:
	{
		PxSphereGeometry& sphereGeometry = geometryHolder.sphere();
		Gizmos::AddSphere(position, sphereGeometry.radius, 8, 8, color);
		break;
	}
	case PxGeometryType::eCAPSULE:
	{
		PxCapsuleGeometry& capsuleGeometry = geometryHolder.capsule();
		Gizmos::AddCapsule(position, capsuleGeometry.halfHeight, capsuleGeometry.radius, 8, 8, color);
	}
	case PxGeometryType::eBOX:
	{
		PxBoxGeometry& boxGeometry = geometryHolder.box();
		Gizmos::AddAABBFilled(position, vec3(boxGeometry.halfExtents.x, boxGeometry.halfExtents.y, boxGeometry.halfExtents.z), color);
		break;
	}
	}
	

	//clear the screen for this frame
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	//display the 3D gizmos
	Gizmos::Draw(m_camera->GetProjectionView());

	//get a orthographic projection matrix and draw 2D gizmos
	int width = 0;
	int height = 0;
	glfwGetWindowSize(m_window, &width, &height);
	glm::mat4 guiMatrix = glm::ortho<float>(0, (float)width, 0, (float)height);

	Gizmos::Draw2D(guiMatrix);
}

void PhysicsApp::DrawGizmosGrid()
{
	//an example of mouse picking
	if (glfwGetMouseButton(m_window, 0) == GLFW_PRESS)
	{
		double x = 0;
		double y = 0;
		glfwGetCursorPos(m_window, &x, &y);

		//plane represents the ground, with a normal of (0,1,0) and a distance of 0 from (0,0,0)
		vec4 plane(0, 1, 0, 0);
		m_pickPosition = m_camera->PickAgainstPlane((float)x, (float)y, plane);
	}
	Gizmos::AddTransform(glm::translate(m_pickPosition));

	//...for now let's add a grid to the gizmos
	for (int i = 0; i < 21; ++i)
	{
		Gizmos::AddLine(vec3(-10 + i, 0, 10), vec3(-10 + i, 0, -10), i == 10 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));

		Gizmos::AddLine(vec3(10, 0, -10 + i), vec3(-10, 0, -10 + i), i == 10 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));
	}
}
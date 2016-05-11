#include "PhysicsApp.h"
#include "gl_core_4_4.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "Camera.h"
#include "Gizmos.h"

#include "Sphere.h"
#include "PhysicsScene.h"
#include "PhysicsRenderer.h"

using glm::vec3;
using glm::vec4;

bool PhysicsApp::Startup()
{
	//create a basic window
	CreateGLFWWindow("AIE OpenGL Application", 1280, 720);

	//start the gizmo system that can draw basic shapes
	Gizmos::Create();

	//create a camera
	m_camera = new Camera(glm::pi<float>() * 0.25f, 16.f / 9.f, 0.1f, 1000.f);
	m_camera->SetLookAtFrom(vec3(10, 10, 10), vec3(0));

	m_pickPosition = vec3(0);

	m_physicsRenderer = new Physics::PhysicsRenderer();
	m_physicsScene = new Physics::PhysicsScene();
	m_objects = m_physicsScene->CreatePhysicsObject<Physics::Sphere>();
	m_objects->SetPosition(vec3(0.f, 10.f, 0.f));
	m_physicsRenderer->GetRenderInfo(m_objects)->color = glm::vec4(1.f, 0.f, 0.f, 0.5f);

	Physics::RigidBody* obj = m_physicsScene->CreatePhysicsObject<Physics::Sphere>();
	m_physicsRenderer->GetRenderInfo(obj)->color = vec4(1.f, 0.f, 0.f, 0.5f);

	return true;
}

void PhysicsApp::Shutdown()
{
	m_physicsScene->DestroyPhysicsObject(m_objects);
	delete m_physicsRenderer;
	delete m_physicsScene;

	//delete our camera and cleanup gizmos
	delete m_camera;
	Gizmos::Destroy();

	//destroy our window properly
	DestroyGLFWWindow();
}

bool PhysicsApp::Update(float deltaTime)
{
	//close the application if the window closes
	if (glfwWindowShouldClose(m_window) || glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		return false;

	//update the camera's movement
	m_camera->Update(deltaTime);

	m_objects->Update(glm::vec3(0, -0.98, 0), deltaTime);
	m_physicsScene->CheckForCollision();

	glm::vec4 forwardVec = m_camera->GetTransform()[2];
	glm::vec4 rightVec = m_camera->GetTransform()[0];

	if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
		m_objects->ApplyForce(glm::vec3(forwardVec * 10.f));
	if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
		m_objects->ApplyForce(glm::vec3(-forwardVec * 10.f));
	if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS)
		m_objects->ApplyForce(glm::vec3(-rightVec * 10.f));
	if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		m_objects->ApplyForce(glm::vec3(rightVec * 10.f));

	m_physicsScene->Simulate(glm::vec3(0, -0.98, 0), deltaTime);

	//return true, else the application closes
	return true;
}

void PhysicsApp::Draw()
{
	//clear the gizmos out for this frame
	Gizmos::Clear();

	DrawGizmosGrid();

	m_physicsRenderer->Render(m_physicsScene);

	//clear the screen for this frame
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	
	//display the 3D gizmos
	Gizmos::Draw(m_camera->GetProjectionView());

	// get a orthographic projection matrix and draw 2D gizmos
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
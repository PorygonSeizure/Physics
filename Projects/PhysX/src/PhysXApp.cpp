#include "PhysXApp.h"
#include "gl_core_4_4.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "Camera.h"
#include "Gizmos.h"

using glm::vec3;
using glm::vec4;
using namespace physx;

bool PhysXApp::Startup()
{
	//create a basic window
	CreateGLFWWindow("AIE OpenGL Application", 1280, 720);

	//start the gizmo system that can draw basic shapes
	Gizmos::Create();

	//create a camera
	m_camera = new FlyCamera(glm::pi<float>() * 0.25f, 16.f / 9.f, 0.1f, 1000.f);
	m_camera->SetLookAtFrom(vec3(10, 10, 10), vec3(0));

	m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);
	PxProfileZoneManager* profileZoneManager = &PxProfileZoneManager::createProfileZoneManager(m_foundation);
	m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, PxTolerancesScale(), true, profileZoneManager);

#ifdef _DEBUG
	if (m_physics->getPvdConnectionManager())
	{
		m_physics->getVisualDebugger()->setVisualizeConstraints(true);
		m_physics->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_CONTACTS, true);
		m_physics->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES, true);
		m_connection = PxVisualDebuggerExt::createConnection(m_physics->getPvdConnectionManager(), "127.0.0.1", 5425, 10);
	}
#endif

	m_wasLeftMousePressed = false;
	m_pickPosition = vec3(0);

	SetupScene();

	return true;
}

void PhysXApp::Shutdown()
{
	m_scene->release();
	m_dispatcher->release();
	PxProfileZoneManager* profileZoneManager = m_physics->getProfileZoneManager();
	if (m_connection != NULL)
		m_connection->release();
	m_physics->release();
	profileZoneManager->release();
	m_foundation->release();

	//delete our camera and cleanup gizmos
	delete m_camera;
	Gizmos::Destroy();

	//destroy our window properly
	DestroyGLFWWindow();
}

void PhysXApp::SetupScene()
{
	PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.f, -9.81f, 0.f);
	m_dispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = m_dispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	m_scene = m_physics->createScene(sceneDesc);

	m_material = m_physics->createMaterial(0.5f, 0.5f, 0.6f);

	PxRigidStatic* groundPlane = PxCreatePlane(*m_physics, PxPlane(0, 1, 0, 0), *m_material);
	m_physXActors.push_back(groundPlane);
	m_scene->addActor(*groundPlane);

	//PxShape* shape = m_physics->createShape(PxBoxGeometry(0.25f, 0.25f, 0.25f), *m_material);
	//PxRigidDynamic* boxActor = m_physics->createRigidDynamic(PxTransform(0.f, 0.25f, 0.f));
	//boxActor->attachShape(*shape);
	//m_physXActors.push_back(boxActor);
	//m_scene->addActor(*boxActor);

	PxShape* shapes[55];
	PxRigidDynamic* boxes[55];
	boxes[0] = m_physics->createRigidDynamic(PxTransform(-1.f, 0.25f, -1.f));
	boxes[1] = m_physics->createRigidDynamic(PxTransform(-0.5f, 0.25f, -1.f));
	boxes[2] = m_physics->createRigidDynamic(PxTransform(0.f, 0.25f, -1.f));
	boxes[3] = m_physics->createRigidDynamic(PxTransform(0.5f, 0.25f, -1.f));
	boxes[4] = m_physics->createRigidDynamic(PxTransform(1.f, 0.25f, -1.f));
	boxes[5] = m_physics->createRigidDynamic(PxTransform(-1.f, 0.25f, -0.5f));
	boxes[6] = m_physics->createRigidDynamic(PxTransform(-0.5f, 0.25f, -0.5f));
	boxes[7] = m_physics->createRigidDynamic(PxTransform(0.f, 0.25f, -0.5f));
	boxes[8] = m_physics->createRigidDynamic(PxTransform(0.5f, 0.25f, -0.5f));
	boxes[9] = m_physics->createRigidDynamic(PxTransform(1.f, 0.25f, -0.5f));
	boxes[10] = m_physics->createRigidDynamic(PxTransform(-1.f, 0.25f, 0.f));
	boxes[11] = m_physics->createRigidDynamic(PxTransform(-0.5f, 0.25f, 0.f));
	boxes[12] = m_physics->createRigidDynamic(PxTransform(0.f, 0.25f, 0.f));
	boxes[13] = m_physics->createRigidDynamic(PxTransform(0.5f, 0.25f, 0.f));
	boxes[14] = m_physics->createRigidDynamic(PxTransform(1.f, 0.25f, 0.f));
	boxes[15] = m_physics->createRigidDynamic(PxTransform(-1.f, 0.25f, 0.5f));
	boxes[16] = m_physics->createRigidDynamic(PxTransform(-0.5f, 0.25f, 0.5f));
	boxes[17] = m_physics->createRigidDynamic(PxTransform(0.f, 0.25f, 0.5f));
	boxes[18] = m_physics->createRigidDynamic(PxTransform(0.5f, 0.25f, 0.5f));
	boxes[19] = m_physics->createRigidDynamic(PxTransform(1.f, 0.25f, 0.5f));
	boxes[20] = m_physics->createRigidDynamic(PxTransform(-1.f, 0.25f, 1.f));
	boxes[21] = m_physics->createRigidDynamic(PxTransform(-0.5f, 0.25f, 1.f));
	boxes[22] = m_physics->createRigidDynamic(PxTransform(0.f, 0.25f, 1.f));
	boxes[23] = m_physics->createRigidDynamic(PxTransform(0.5f, 0.25f, 1.f));
	boxes[24] = m_physics->createRigidDynamic(PxTransform(1.f, 0.25f, 1.f));

	boxes[25] = m_physics->createRigidDynamic(PxTransform(-0.75f, 0.75f, -0.75f));
	boxes[26] = m_physics->createRigidDynamic(PxTransform(-0.25f, 0.75f, -0.75f));
	boxes[27] = m_physics->createRigidDynamic(PxTransform(0.25f, 0.75f, -0.75f));
	boxes[28] = m_physics->createRigidDynamic(PxTransform(0.75f, 0.75f, -0.75f));
	boxes[29] = m_physics->createRigidDynamic(PxTransform(-0.75f, 0.75f, -0.25f));
	boxes[30] = m_physics->createRigidDynamic(PxTransform(-0.25f, 0.75f, -0.25f));
	boxes[31] = m_physics->createRigidDynamic(PxTransform(0.25f, 0.75f, -0.25f));
	boxes[32] = m_physics->createRigidDynamic(PxTransform(0.75f, 0.75f, -0.25f));
	boxes[33] = m_physics->createRigidDynamic(PxTransform(-0.75f, 0.75f, 0.25f));
	boxes[34] = m_physics->createRigidDynamic(PxTransform(-0.25f, 0.75f, 0.25f));
	boxes[35] = m_physics->createRigidDynamic(PxTransform(0.25f, 0.75f, 0.25f));
	boxes[36] = m_physics->createRigidDynamic(PxTransform(0.75f, 0.75f, 0.25f));
	boxes[37] = m_physics->createRigidDynamic(PxTransform(-0.75f, 0.75f, 0.75f));
	boxes[38] = m_physics->createRigidDynamic(PxTransform(-0.25f, 0.75f, 0.75f));
	boxes[39] = m_physics->createRigidDynamic(PxTransform(0.25f, 0.75f, 0.75f));
	boxes[40] = m_physics->createRigidDynamic(PxTransform(0.75f, 0.75f, 0.75f));

	boxes[41] = m_physics->createRigidDynamic(PxTransform(-0.5f, 1.25f, -0.5f));
	boxes[42] = m_physics->createRigidDynamic(PxTransform(0.f, 1.25f, -0.5f));
	boxes[43] = m_physics->createRigidDynamic(PxTransform(0.5f, 1.25f, -0.5f));
	boxes[44] = m_physics->createRigidDynamic(PxTransform(-0.5f, 1.25f, 0.f));
	boxes[45] = m_physics->createRigidDynamic(PxTransform(0.f, 1.25f, 0.f));
	boxes[46] = m_physics->createRigidDynamic(PxTransform(0.5f, 1.25f, 0.f));
	boxes[47] = m_physics->createRigidDynamic(PxTransform(-0.5f, 1.25f, 0.5f));
	boxes[48] = m_physics->createRigidDynamic(PxTransform(0.f, 1.25f, 0.5f));
	boxes[49] = m_physics->createRigidDynamic(PxTransform(0.5f, 1.25f, 0.5f));

	boxes[50] = m_physics->createRigidDynamic(PxTransform(-0.25f, 1.75f, -0.25f));
	boxes[51] = m_physics->createRigidDynamic(PxTransform(0.25f, 1.75f, -0.25f));
	boxes[52] = m_physics->createRigidDynamic(PxTransform(-0.25f, 1.75f, 0.25f));
	boxes[53] = m_physics->createRigidDynamic(PxTransform(0.25f, 1.75f, 0.25f));

	boxes[54] = m_physics->createRigidDynamic(PxTransform(0.f, 2.25f, 0.f));

	for (int i = 0; i < 55; i++)
	{
		shapes[i] = m_physics->createShape(PxBoxGeometry(0.25f, 0.25f, 0.25f), *m_material);
		boxes[i]->attachShape(*shapes[i]);
		m_physXActors.push_back(boxes[i]);
		m_scene->addActor(*boxes[i]);
	}
}

bool PhysXApp::Update(float deltaTime)
{
	//close the application if the window closes
	if (glfwWindowShouldClose(m_window) || glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		return false;

	//update the camera's movement
	m_camera->Update(deltaTime);

	bool leftMousePressed = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS);

	m_scene->simulate(deltaTime);
	m_scene->fetchResults(true);
	//PxTransform transform = boxActor->getGlobalPose();

	//printf("%0.2f\t%0.2f\t%0.2f\n", transform.p.x, transform.p.y, transform.p.z);

	if (leftMousePressed && !m_wasLeftMousePressed)
	{
		vec3 startPos(m_camera->GetTransform()[3]);
		vec3 direction(m_camera->GetTransform()[2]);
		PxVec3 velocity = PxVec3(direction.x, direction.y, direction.z) * -25.f;

		PxShape* shape = m_physics->createShape(PxBoxGeometry(0.25f, 0.25f, 0.25f), *m_material);
		PxRigidDynamic* boxActor = m_physics->createRigidDynamic(PxTransform(startPos.x, startPos.y, startPos.z));
		boxActor->setLinearVelocity(velocity);
		boxActor->attachShape(*shape);
		m_physXActors.push_back(boxActor);
		m_scene->addActor(*boxActor);

		m_wasLeftMousePressed = true;
	}
	else if (!leftMousePressed && m_wasLeftMousePressed)
		m_wasLeftMousePressed = false;

	//return true, else the application closes
	return true;
}

void PhysXApp::Draw()
{
	//clear the gizmos out for this frame
	Gizmos::Clear();

	//DrawGizmosGrid();

	for (auto actor : m_physXActors)
	{
		PxU32 numberShapes = actor->getNbShapes();
		PxShape** shapes = new PxShape*[numberShapes];
		actor->getShapes(shapes, numberShapes);

		while (numberShapes--)
			AddWidget(shapes[numberShapes], actor);
		delete[] shapes;
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

void PhysXApp::DrawGizmosGrid()
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

void PhysXApp::AddWidget(PxShape* shape, PxRigidActor* actor)
{
	PxGeometryType::Enum type = shape->getGeometryType();
	switch (type)
	{
		case PxGeometryType::eSPHERE:
			AddSphere(shape, actor);
			break;
		case PxGeometryType::ePLANE:
			AddPlane(shape, actor);
			break;
		case PxGeometryType::eCAPSULE:
			//AddCapsule(shape, actor);
			break;
		case PxGeometryType::eBOX:
			AddBox(shape, actor);
			break;
		default:	break;
	}
}

void PhysXApp::AddSphere(PxShape* shape, PxRigidActor* actor)
{
	PxSphereGeometry geometry;
	float radius = 1;

	bool status = shape->getSphereGeometry(geometry);
	if (status)
		radius = geometry.radius;

	PxMat44 pxm(PxShapeExt::getGlobalPose(*shape, *actor));
	glm::mat4 m(pxm.column0.x, pxm.column0.y, pxm.column0.z, pxm.column0.w,
		pxm.column1.x, pxm.column1.y, pxm.column1.z, pxm.column1.w,
		pxm.column2.x, pxm.column2.y, pxm.column2.z, pxm.column2.w,
		pxm.column3.x, pxm.column3.y, pxm.column3.z, pxm.column3.w);
	vec3 position;

	position.x = pxm.getPosition().x;
	position.y = pxm.getPosition().y;
	position.z = pxm.getPosition().z;
	vec4 colour = vec4(1, 0, 0, 1);
	if (actor->getName() != NULL && strcmp(actor->getName(), "Pickup1"))
		colour = vec4(0, 1, 0, 1);

	Gizmos::AddSphere(position, radius, 8, 8, colour, &m);
}

void DrawPlane(vec3 position, vec3 normal, vec4 colour)
{
	vec3 xVert(1, 0, 0);
	vec3 yVert(0, 1, 0);
	vec3 zVert(0, 0, 1);

	vec3 vector1;
	vec3 vector2;

	if (normal == xVert)
	{
		vector1 = yVert;
		vector2 = zVert;
	}
	else if (normal == yVert)
	{
		vector1 = zVert;
		vector2 = xVert;
	}
	else if (normal == zVert)
	{
		vector1 = xVert;
		vector2 = yVert;
	}

	vector1 *= 100.f;
	vector2 *= 100.f;
	vec3 one = position - vector1 - vector2;
	vec3 two = position + vector1 - vector2;
	vec3 three = position + vector1 + vector2;
	vec3 four = position - vector1 + vector2;

	Gizmos::AddLine(one, two, colour);
	Gizmos::AddLine(two, three, colour);
	Gizmos::AddLine(three, four, colour);
	Gizmos::AddLine(four, one, colour);

	vec3 min = glm::min(three, one);
	vec3 dif = glm::abs(three - one);
	int lineVar = 1;
	glm::ivec3 num = dif / lineVar;
	for (int i = 0; i < num.x; i++)
		Gizmos::AddLine(vec3(min.x + (i * lineVar), min.y, min.z), vec3(min.x + (i * lineVar), -min.y, -min.z), i % 10 == 0 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));
	for (int i = 0; i < num.y; i++)
		Gizmos::AddLine(vec3(min.x, min.y + (i * lineVar), min.z), vec3(-min.x, min.y + (i * lineVar), -min.z), i % 10 == 0 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));
	for (int i = 0; i < num.z; i++)
		Gizmos::AddLine(vec3(min.x, min.y, min.z + (i * lineVar)), vec3(-min.x, -min.y, min.z + (i * lineVar)), i % 10 == 0 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));
}

void PhysXApp::AddPlane(PxShape* shape, PxRigidActor* actor)
{
	PxPlaneGeometry geometry;

	PxMat44 pxm(PxShapeExt::getGlobalPose(*shape, *actor));
	glm::mat4 m(pxm.column0.x, pxm.column0.y, pxm.column0.z, pxm.column0.w,
				pxm.column1.x, pxm.column1.y, pxm.column1.z, pxm.column1.w,
				pxm.column2.x, pxm.column2.y, pxm.column2.z, pxm.column2.w,
				pxm.column3.x, pxm.column3.y, pxm.column3.z, pxm.column3.w);
	vec3 position;
	position.x = pxm.getPosition().x;
	position.y = pxm.getPosition().y;
	position.z = pxm.getPosition().z;

	vec3 normal(0);

	bool status = shape->getPlaneGeometry(geometry);
	if (status)
	{
		PxPlane plane = PxPlaneEquationFromTransform(PxTransform(pxm));
		normal = vec3((int)plane.n.x, (int)plane.n.y, (int)plane.n.z);
	}

	vec4 colour = vec4(1, 0, 0, 1);
	if (actor->getName() != NULL && strcmp(actor->getName(), "Pickup1"))
		colour = vec4(0, 1, 0, 1);

	DrawPlane(position, normal, colour);
	//Gizmos::AddAABB(position, vec3(100.f * m[0].x, 100.f * m[1].y, 100.f * m[2].z), colour, &m);
}

void PhysXApp::AddBox(PxShape* shape, PxRigidActor* actor)
{
	PxBoxGeometry geometry;
	float width = 1;
	float height = 1;
	float length = 1;
	bool status = shape->getBoxGeometry(geometry);
	if (status)
	{
		width = geometry.halfExtents.x;
		height = geometry.halfExtents.y;
		length = geometry.halfExtents.z;
	}

	PxMat44 pxm(PxShapeExt::getGlobalPose(*shape, *actor));
	glm::mat4 m(pxm.column0.x, pxm.column0.y, pxm.column0.z, pxm.column0.w,
		pxm.column1.x, pxm.column1.y, pxm.column1.z, pxm.column1.w,
		pxm.column2.x, pxm.column2.y, pxm.column2.z, pxm.column2.w,
		pxm.column3.x, pxm.column3.y, pxm.column3.z, pxm.column3.w);
	vec3 position;

	position.x = pxm.getPosition().x;
	position.y = pxm.getPosition().y;
	position.z = pxm.getPosition().z;
	vec3 extents = vec3(width, height, length);
	vec4 colour = vec4(1, 0, 0, 1);
	if (actor->getName() != NULL && strcmp(actor->getName(), "Pickup1"))
		colour = vec4(0, 1, 0, 1);

	Gizmos::AddAABBFilled(position, extents, colour, &m);
}
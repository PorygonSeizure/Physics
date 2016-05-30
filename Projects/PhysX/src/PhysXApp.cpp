#include "PhysXApp.h"
#include "gl_core_4_4.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "Camera.h"
#include "Gizmos.h"
#include "Ragdoll.h"
#include "Collision.h"
#include "Mesh.h"
#include <Shader.h>

using glm::vec3;
using glm::vec4;
using glm::mat4;
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

	//m_shaders = new Shader();
	//CreateShader();
	//m_mesh = new Mesh();
	//m_mesh->LoadObj("./res/Soulspear/soulspear.obj");
	
	SetupScene();

	return true;
}

void PhysXApp::Shutdown()
{
	m_scene->release();
	m_dispatcher->release();
	//delete m_mesh;
	//delete m_shaders;
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

PxFilterFlags FilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags,
							const void* constantBlock, PxU32 constantBlockSize)
{
	//let triggers through
	if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		return PxFilterFlag::eDEFAULT;
	}
	//generate contacts for all that were not filtered above
	pairFlags = PxPairFlag::eCONTACT_DEFAULT;
	//trigger the contact callback for pairs (A,B) where
	//the filtermask of A contains the ID of B and vice versa.
	if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_LOST;
	return PxFilterFlag::eDEFAULT;
}

void SetupFiltering(PxRigidActor* actor, PxU32 filterGroup, PxU32 filterMask)
{
	PxFilterData filterData;
	filterData.word0 = filterGroup;	//word0 = own ID
	filterData.word1 = filterMask;	//word1 = ID mask to filter pairs that trigger a contact callback
	const PxU32 numberShapes = actor->getNbShapes();
	PxShape** shapes = (PxShape**)_aligned_malloc(sizeof(PxShape*)*numberShapes, 16);
	actor->getShapes(shapes, numberShapes);
	for (PxU32 i = 0; i < numberShapes; i++)
	{
		PxShape* shape = shapes[i];
		shape->setSimulationFilterData(filterData);
	}
	_aligned_free(shapes);
}

void SetShapeAsTrigger(PxRigidActor* actorIn)
{
	PxRigidStatic* staticActor = actorIn->is<PxRigidStatic>();
	//assert(staticActor);
	if (staticActor != nullptr)
	{
		const PxU32 numberShapes = staticActor->getNbShapes();
		PxShape** shapes = (PxShape**)_aligned_malloc(sizeof(PxShape*)*numberShapes, 16);
		staticActor->getShapes(shapes, numberShapes);
		for (PxU32 i = 0; i < numberShapes; i++)
		{
			shapes[i]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
			shapes[i]->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
		}
	}
}

void PhysXApp::SetupScene()
{
	PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.f, -9.81f, 0.f);
	m_dispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = m_dispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	m_scene = m_physics->createScene(sceneDesc);
	PxSimulationEventCallback* collisionEventCallback = new Collision();
	m_scene->setSimulationEventCallback(collisionEventCallback);
	sceneDesc.filterShader = FilterShader;

	m_material = m_physics->createMaterial(0.5f, 0.5f, 0.6f);

	PxRigidStatic* groundPlane = PxCreatePlane(*m_physics, PxPlane(0, 1, 0, 0), *m_material);
	//PxRigidStatic* groundPlane = PxCreatePlane(*m_physics, PxPlane((1.f / sqrtf(2.f)), (1.f / sqrtf(2.f)), 0, 0), *m_material);
	m_physXActors.push_back(groundPlane);
	m_scene->addActor(*groundPlane);
	groundPlane->setName("ground");
	SetupFiltering(groundPlane, FilterGroup::eGROUND, FilterGroup::ePLAYER);
	//SetShapeAsTrigger(groundPlane);

	//PxShape* shape = m_physics->createShape(PxSphereGeometry(0.25f), *m_material);
	//PxRigidDynamic* sphereActor = m_physics->createRigidDynamic(PxTransform(-5.f, 0.25f, -5.f));
	//sphereActor->attachShape(*shape);
	//m_physXActors.push_back(sphereActor);
	//m_scene->addActor(*sphereActor);

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
		char buffer[32];
		sprintf(buffer, "box%i", i);

		shapes[i] = m_physics->createShape(PxBoxGeometry(0.25f, 0.25f, 0.25f), *m_material);
		boxes[i]->attachShape(*shapes[i]);
		m_physXActors.push_back(boxes[i]);
		m_scene->addActor(*boxes[i]);
		boxes[i]->setName(buffer);
		SetupFiltering(boxes[i], FilterGroup::ePLAYER, FilterGroup::eGROUND);
		SetupFiltering(boxes[i], FilterGroup::ePLAYER, FilterGroup::eGROUND | FilterGroup::ePLATFORM);
	}

	//create some constants for axis of rotation to make definition of quaternions a bit neater
	const physx::PxVec3 X_AXIS = physx::PxVec3(1, 0, 0);
	const physx::PxVec3 Y_AXIS = physx::PxVec3(0, 1, 0);
	const physx::PxVec3 Z_AXIS = physx::PxVec3(0, 0, 1);

	RagdollNode* ragdollData[] =
	{
		new RagdollNode(PxQuat(PxPi / 2.f, Z_AXIS), NO_PARENT, 1, 3, 1, 1, "lower spine"),
		new RagdollNode(PxQuat(PxPi, Z_AXIS), LOWER_SPINE, 1, 1, -1, 1, "left pelvis"),
		new RagdollNode(PxQuat(0, Z_AXIS), LOWER_SPINE, 1, 1, -1, 1, "right pelvis"),
		new RagdollNode(PxQuat(PxPi / 2.f + 0.2f, Z_AXIS), LEFT_PELVIS, 5, 2, -1, 1, "L upper leg"),
		new RagdollNode(PxQuat(PxPi / 2.f - 0.2f, Z_AXIS), RIGHT_PELVIS, 5, 2, -1, 1, "R upper leg"),
		new RagdollNode(PxQuat(PxPi / 2.f + 0.2f, Z_AXIS), LEFT_UPPER_LEG, 5, 1.75, -1, 1, "L lower leg"),
		new RagdollNode(PxQuat(PxPi / 2.f - 0.2f, Z_AXIS), RIGHT_UPPER_LEG, 5, 1.75, -1, 1, "R lowerleg"),
		new RagdollNode(PxQuat(PxPi / 2.f, Z_AXIS), LOWER_SPINE, 1, 3, 1, -1, "upper spine"),
		new RagdollNode(PxQuat(PxPi, Z_AXIS), UPPER_SPINE, 1, 1.5, 1, 1, "left clavicle"),
		new RagdollNode(PxQuat(0, Z_AXIS), UPPER_SPINE, 1, 1.5, 1, 1, "right clavicle"),
		new RagdollNode(PxQuat(PxPi / 2.f, Z_AXIS), UPPER_SPINE, 1, 1, 1, -1, "neck"),
		new RagdollNode(PxQuat(PxPi / 2.f, Z_AXIS), NECK, 1, 3, 1, -1, "HEAD"),
		new RagdollNode(PxQuat(PxPi - 0.3f, Z_AXIS), LEFT_CLAVICLE, 3, 1.5, -1, 1, "left upper arm"),
		new RagdollNode(PxQuat(0.3f, Z_AXIS), RIGHT_CLAVICLE, 3, 1.5, -1, 1, "right upper arm"),
		new RagdollNode(PxQuat(PxPi - 0.3f, Z_AXIS), LEFT_UPPER_ARM, 3, 1, -1, 1, "left lower arm"),
		new RagdollNode(PxQuat(0.3f, Z_AXIS), RIGHT_UPPER_ARM, 3, 1, -1, 1, "right lower arm"),
		NULL
	};

	PxArticulation* ragdollArticulation = MakeRagdoll(m_physics, ragdollData, PxTransform(PxVec3(5, 5, 5)), 0.1f, m_material);
	m_ragdolls.push_back(ragdollArticulation);
	m_scene->addArticulation(*ragdollArticulation);
}

bool PhysXApp::Update(float deltaTime)
{
	//close the application if the window closes
	if (glfwWindowShouldClose(m_window) || glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		return false;

	//update the camera's movement
	m_camera->Update(deltaTime);

	bool leftMousePressed = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS);

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

	m_scene->simulate(deltaTime);
	m_scene->fetchResults(true);

	//return true, else the application closes
	return true;
}

void PhysXApp::Draw()
{
	//clear the gizmos out for this frame
	Gizmos::Clear();

	for (auto actor : m_physXActors)
	{
		PxU32 numberShapes = actor->getNbShapes();
		PxShape** shapes = new PxShape*[numberShapes];
		actor->getShapes(shapes, numberShapes);

		while (numberShapes--)
			AddWidget(shapes[numberShapes], actor);
		delete[] shapes;
	}

	for (auto artiulcation : m_ragdolls)
	{
		PxU32 numberLinks = artiulcation->getNbLinks();
		PxArticulationLink** links = new PxArticulationLink*[numberLinks];
		artiulcation->getLinks(links, numberLinks);

		while (numberLinks--)
		{
			PxArticulationLink* link = links[numberLinks];
			PxU32 numberShapes = link->getNbShapes();
			PxShape** shapes = new PxShape*[numberShapes];
			link->getShapes(shapes, numberShapes);
			while (numberShapes--)
				AddWidget(shapes[numberShapes], link);
		}

		delete[] links;
	}

	//clear the screen for this frame
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//m_shaders->Bind();
	//
	//int loc = m_shaders->GetUniform("projectionView");
	//glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(m_camera->GetProjectionView()));
	//
	//loc = m_shaders->GetUniform("cameraPosition");
	//glUniform3fv(loc, 1, value_ptr(vec3(m_camera->GetTransform()[0].x, m_camera->GetTransform()[1].y, m_camera->GetTransform()[2].z)));
	//loc = m_shaders->GetUniform("lightPosition");
	//glUniform3fv(loc, 1, value_ptr(vec3(1, 1, 1)));
	//
	//m_mesh->Draw(GL_TRIANGLES);
	
	//display the 3D gizmos
	Gizmos::Draw(m_camera->GetProjectionView());

	//get a orthographic projection matrix and draw 2D gizmos
	int width = 0;
	int height = 0;
	glfwGetWindowSize(m_window, &width, &height);
	mat4 guiMatrix = glm::ortho<float>(0, (float)width, 0, (float)height);

	Gizmos::Draw2D(guiMatrix);
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
			AddCapsule(shape, actor);
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
	mat4 m(pxm.column0.x, pxm.column0.y, pxm.column0.z, pxm.column0.w,
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

	Gizmos::AddSphere(position, radius, 8, 8, colour, nullptr, 0.f, 360.f, -90.f, 90.f);
}

void DrawPlane(vec3 position, vec3 normal, vec4 colour)
{
	if (normal.x < 0.f)
		normal.x = 0.f;
	if (normal.y < 0.f)
		normal.y = 0.f;
	if (normal.z < 0.f)
		normal.z = 0.f;
	vec3 vert(0, 0, 1);

	if (glm::length(glm::cross(vert, normal)) < (1.f / 360.f))
		vert = vec3(0, 1, 0);
	if (glm::length(glm::cross(vert, normal)) < (1.f / 360.f))
		vert = vec3(1, 0, 0);
	
	vec3 vector1 = glm::normalize(glm::cross(normal, vert));
	vec3 vector2 = glm::cross(vector1, normal);

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

	//vec3 min = glm::min(three, one);
	//vec3 dif = glm::abs(three - one);
	//int lineVar = 1;
	//glm::ivec3 num = dif / lineVar;
	//for (int i = 0; i < num.x; i++)
	//	Gizmos::AddLine(vec3(min.x + (i * lineVar), min.y, min.z), vec3(min.x + (i * lineVar), -min.y, -min.z), i % 10 == 0 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));
	//for (int i = 0; i < num.y; i++)
	//	Gizmos::AddLine(vec3(min.x, min.y + (i * lineVar), min.z), vec3(-min.x, min.y + (i * lineVar), -min.z), i % 10 == 0 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));
	//for (int i = 0; i < num.z; i++)
	//	Gizmos::AddLine(vec3(min.x, min.y, min.z + (i * lineVar)), vec3(-min.x, -min.y, min.z + (i * lineVar)), i % 10 == 0 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));

	vec3 dif = four - one;
	int lineVar = 1;
	vec3 num = dif / lineVar;
	float max = glm::max(num.x, num.y, num.z);
	vec3 step = num / max;
	
	for (float i = 0.f, j = 0.f, k = 0.f; max == num.x ? i < num.x : max == num.y ? j < num.y : k < num.z; i += step.x, j += step.y, k += step.z)
	{
		vec3 p1(one.x + (i * lineVar), one.y + (j * lineVar), one.z + (k * lineVar));
		vec3 p2(two.x + (i * lineVar), two.y + (j * lineVar), two.z + (k * lineVar));
	
		Gizmos::AddLine(p1, p2, vec4(0, 0, 0, 1));
	}
		
	dif = two - one;
	num = dif / lineVar;
	max = glm::max(num.x, num.y, num.z);
	step = num / max;

	for (float i = 0.f, j = 0.f, k = 0.f; max == num.x ? i < num.x : max == num.y ? j < num.y : k < num.z; i += step.x, j += step.y, k += step.z)
	{
		vec3 p1(one.x + (i * lineVar), one.y + (j * lineVar), one.z + (k * lineVar));
		vec3 p2(four.x + (i * lineVar), four.y + (j * lineVar), four.z + (k * lineVar));
	
		Gizmos::AddLine(p1, p2, vec4(0, 0, 0, 1));
	}
}

void PhysXApp::AddPlane(PxShape* shape, PxRigidActor* actor)
{
	PxPlaneGeometry geometry;

	PxMat44 pxm(PxShapeExt::getGlobalPose(*shape, *actor));
	mat4 m(pxm.column0.x, pxm.column0.y, pxm.column0.z, pxm.column0.w,
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
		normal = vec3(plane.n.x, plane.n.y, plane.n.z);
	}

	vec4 colour = vec4(1, 0, 0, 1);
	if (actor->getName() != NULL && strcmp(actor->getName(), "Pickup1"))
		colour = vec4(0, 1, 0, 1);

	DrawPlane(position, normal, colour);
}

void PhysXApp::AddCapsule(PxShape* shape, PxRigidActor* actor)
{
	PxCapsuleGeometry geometry;
	float radius = 1.f;
	float halfHeight = 2.f;

	bool status = shape->getCapsuleGeometry(geometry);
	if (status)
	{
		radius = geometry.radius;
		halfHeight = geometry.halfHeight;
	}

	PxMat44 pxm(PxShapeExt::getGlobalPose(*shape, *actor));
	mat4 m(pxm.column0.x, pxm.column0.y, pxm.column0.z, pxm.column0.w,
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

	Gizmos::AddCapsule(position, radius, halfHeight, 8, 8, colour, &m);
}

void PhysXApp::AddBox(PxShape* shape, PxRigidActor* actor)
{
	PxBoxGeometry geometry;
	float width = 1.f;
	float height = 1.f;
	float length = 1.f;
	bool status = shape->getBoxGeometry(geometry);
	if (status)
	{
		width = geometry.halfExtents.x;
		height = geometry.halfExtents.y;
		length = geometry.halfExtents.z;
	}

	PxMat44 pxm(PxShapeExt::getGlobalPose(*shape, *actor));
	mat4 m(pxm.column0.x, pxm.column0.y, pxm.column0.z, pxm.column0.w,
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

void PhysXApp::CreateShader()
{
	m_shaders->LoadShader(GL_VERTEX_SHADER, "./res/OBJVertex.vs");
	m_shaders->LoadShader(GL_FRAGMENT_SHADER, "./res/OBJFragment.fs");
	m_shaders->Link();
}
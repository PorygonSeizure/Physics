#include "PhysXApp.h"
#include <gl_core_4_4.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "Camera.h"
#include "Gizmos.h"
#include "Ragdoll.h"
#include "Collision.h"
#include "Mesh.h"
#include <Shader.h>
//#include "FBXFile.h"
#include "ParticleEmitter.h"
#include "ParticleFluidEmitter.h"

using glm::vec3;
using physx::PxProfileZoneManager;
using physx::PxVec3;
using physx::PxRigidStatic;
using physx::PxTransform;
using physx::PxQuat;
using physx::PxU32;
using physx::PxBoxGeometry;
using std::vector;
using physx::PxPi;
using physx::PxShape;
using physx::PxArticulationLink;
using glm::mat4;
using physx::PxRigidActor;
using physx::PxGeometryType;
using physx::PxMat44;
using physx::PxShapeExt;
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
	m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, physx::PxTolerancesScale(), true, profileZoneManager);

#ifdef _DEBUG
	if (m_physics->getPvdConnectionManager())
	{
		m_physics->getVisualDebugger()->setVisualizeConstraints(true);
		m_physics->getVisualDebugger()->setVisualDebuggerFlag(physx::PxVisualDebuggerFlag::eTRANSMIT_CONTACTS, true);
		m_physics->getVisualDebugger()->setVisualDebuggerFlag(physx::PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES, true);
		m_connection = physx::PxVisualDebuggerExt::createConnection(m_physics->getPvdConnectionManager(), "127.0.0.1", 5425, 10);
	}
#endif

	m_wasLeftMousePressed = false;
	m_pickPosition = vec3(0);

	m_shaders = new Shader();
	CreateShader();
	m_mesh = new Mesh();
	m_mesh->LoadObj("./res/tank/battle_tank.obj");

	//m_FBX = new FBXFile();
	//m_FBX->Load("./res/Soulspear/soulspear.fbx");
	//CreateOpenGLBuffers();
	m_cooker = PxCreateCooking(PX_PHYSICS_VERSION, *m_foundation, PxCookingParams(PxTolerancesScale()));
	
	SetupScene();

	PxRigidDynamic* tank = nullptr;
	//AttachedRigidBodyConvex(10.f, m_material, tank);

	return true;
}

void PhysXApp::Shutdown()
{
	delete m_particleFluidEmitter;
	delete m_collisionEventCallback;
	m_scene->release();
	m_dispatcher->release();
	m_cooker->release();
	//CleanupOpenGLBuffers();
	//delete m_FBX;
	delete m_mesh;
	delete m_shaders;
	PxProfileZoneManager* profileZoneManager = m_physics->getProfileZoneManager();
	if (m_connection)
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
	physx::PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.f, -9.81f, 0.f);
	m_dispatcher = physx::PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = m_dispatcher;
	sceneDesc.filterShader = FilterShader;
	m_scene = m_physics->createScene(sceneDesc);
	m_collisionEventCallback = new Collision();
	m_scene->setSimulationEventCallback(m_collisionEventCallback);

	m_material = m_physics->createMaterial(0.5f, 0.5f, 0.6f);

	PxRigidStatic* groundPlane = PxCreatePlane(*m_physics, physx::PxPlane(0, 1, 0, 0), *m_material);
	//PxRigidStatic* groundPlane = PxCreatePlane(*m_physics, physx::PxPlane((1.f / sqrtf(2.f)), (1.f / sqrtf(2.f)), 0, 0), *m_material);
	m_physXActors.push_back(groundPlane);
	m_scene->addActor(*groundPlane);
	groundPlane->setName("ground");
	SetupFiltering(groundPlane, FilterGroup::eGROUND, FilterGroup::ePLAYER);
	SetShapeAsTrigger(groundPlane);

	//PxShape* shapes[55];
	//physx::PxRigidDynamic* boxes[55];
	//boxes[0] = m_physics->createRigidDynamic(PxTransform(-1.f, 0.25f, -1.f));
	//boxes[1] = m_physics->createRigidDynamic(PxTransform(-0.5f, 0.25f, -1.f));
	//boxes[2] = m_physics->createRigidDynamic(PxTransform(0.f, 0.25f, -1.f));
	//boxes[3] = m_physics->createRigidDynamic(PxTransform(0.5f, 0.25f, -1.f));
	//boxes[4] = m_physics->createRigidDynamic(PxTransform(1.f, 0.25f, -1.f));
	//boxes[5] = m_physics->createRigidDynamic(PxTransform(-1.f, 0.25f, -0.5f));
	//boxes[6] = m_physics->createRigidDynamic(PxTransform(-0.5f, 0.25f, -0.5f));
	//boxes[7] = m_physics->createRigidDynamic(PxTransform(0.f, 0.25f, -0.5f));
	//boxes[8] = m_physics->createRigidDynamic(PxTransform(0.5f, 0.25f, -0.5f));
	//boxes[9] = m_physics->createRigidDynamic(PxTransform(1.f, 0.25f, -0.5f));
	//boxes[10] = m_physics->createRigidDynamic(PxTransform(-1.f, 0.25f, 0.f));
	//boxes[11] = m_physics->createRigidDynamic(PxTransform(-0.5f, 0.25f, 0.f));
	//boxes[12] = m_physics->createRigidDynamic(PxTransform(0.f, 0.25f, 0.f));
	//boxes[13] = m_physics->createRigidDynamic(PxTransform(0.5f, 0.25f, 0.f));
	//boxes[14] = m_physics->createRigidDynamic(PxTransform(1.f, 0.25f, 0.f));
	//boxes[15] = m_physics->createRigidDynamic(PxTransform(-1.f, 0.25f, 0.5f));
	//boxes[16] = m_physics->createRigidDynamic(PxTransform(-0.5f, 0.25f, 0.5f));
	//boxes[17] = m_physics->createRigidDynamic(PxTransform(0.f, 0.25f, 0.5f));
	//boxes[18] = m_physics->createRigidDynamic(PxTransform(0.5f, 0.25f, 0.5f));
	//boxes[19] = m_physics->createRigidDynamic(PxTransform(1.f, 0.25f, 0.5f));
	//boxes[20] = m_physics->createRigidDynamic(PxTransform(-1.f, 0.25f, 1.f));
	//boxes[21] = m_physics->createRigidDynamic(PxTransform(-0.5f, 0.25f, 1.f));
	//boxes[22] = m_physics->createRigidDynamic(PxTransform(0.f, 0.25f, 1.f));
	//boxes[23] = m_physics->createRigidDynamic(PxTransform(0.5f, 0.25f, 1.f));
	//boxes[24] = m_physics->createRigidDynamic(PxTransform(1.f, 0.25f, 1.f));
	//
	//boxes[25] = m_physics->createRigidDynamic(PxTransform(-0.75f, 0.75f, -0.75f));
	//boxes[26] = m_physics->createRigidDynamic(PxTransform(-0.25f, 0.75f, -0.75f));
	//boxes[27] = m_physics->createRigidDynamic(PxTransform(0.25f, 0.75f, -0.75f));
	//boxes[28] = m_physics->createRigidDynamic(PxTransform(0.75f, 0.75f, -0.75f));
	//boxes[29] = m_physics->createRigidDynamic(PxTransform(-0.75f, 0.75f, -0.25f));
	//boxes[30] = m_physics->createRigidDynamic(PxTransform(-0.25f, 0.75f, -0.25f));
	//boxes[31] = m_physics->createRigidDynamic(PxTransform(0.25f, 0.75f, -0.25f));
	//boxes[32] = m_physics->createRigidDynamic(PxTransform(0.75f, 0.75f, -0.25f));
	//boxes[33] = m_physics->createRigidDynamic(PxTransform(-0.75f, 0.75f, 0.25f));
	//boxes[34] = m_physics->createRigidDynamic(PxTransform(-0.25f, 0.75f, 0.25f));
	//boxes[35] = m_physics->createRigidDynamic(PxTransform(0.25f, 0.75f, 0.25f));
	//boxes[36] = m_physics->createRigidDynamic(PxTransform(0.75f, 0.75f, 0.25f));
	//boxes[37] = m_physics->createRigidDynamic(PxTransform(-0.75f, 0.75f, 0.75f));
	//boxes[38] = m_physics->createRigidDynamic(PxTransform(-0.25f, 0.75f, 0.75f));
	//boxes[39] = m_physics->createRigidDynamic(PxTransform(0.25f, 0.75f, 0.75f));
	//boxes[40] = m_physics->createRigidDynamic(PxTransform(0.75f, 0.75f, 0.75f));
	//
	//boxes[41] = m_physics->createRigidDynamic(PxTransform(-0.5f, 1.25f, -0.5f));
	//boxes[42] = m_physics->createRigidDynamic(PxTransform(0.f, 1.25f, -0.5f));
	//boxes[43] = m_physics->createRigidDynamic(PxTransform(0.5f, 1.25f, -0.5f));
	//boxes[44] = m_physics->createRigidDynamic(PxTransform(-0.5f, 1.25f, 0.f));
	//boxes[45] = m_physics->createRigidDynamic(PxTransform(0.f, 1.25f, 0.f));
	//boxes[46] = m_physics->createRigidDynamic(PxTransform(0.5f, 1.25f, 0.f));
	//boxes[47] = m_physics->createRigidDynamic(PxTransform(-0.5f, 1.25f, 0.5f));
	//boxes[48] = m_physics->createRigidDynamic(PxTransform(0.f, 1.25f, 0.5f));
	//boxes[49] = m_physics->createRigidDynamic(PxTransform(0.5f, 1.25f, 0.5f));
	//
	//boxes[50] = m_physics->createRigidDynamic(PxTransform(-0.25f, 1.75f, -0.25f));
	//boxes[51] = m_physics->createRigidDynamic(PxTransform(0.25f, 1.75f, -0.25f));
	//boxes[52] = m_physics->createRigidDynamic(PxTransform(-0.25f, 1.75f, 0.25f));
	//boxes[53] = m_physics->createRigidDynamic(PxTransform(0.25f, 1.75f, 0.25f));
	//
	//boxes[54] = m_physics->createRigidDynamic(PxTransform(0.f, 2.25f, 0.f));
	//
	//for (int i = 0; i < 55; i++)
	//{
	//	char buffer[32];
	//	sprintf(buffer, "box%i", i);
	//
	//	shapes[i] = m_physics->createShape(PxBoxGeometry(0.25f, 0.25f, 0.25f), *m_material);
	//	boxes[i]->attachShape(*shapes[i]);
	//	m_physXActors.push_back(boxes[i]);
	//	m_scene->addActor(*boxes[i]);
	//	//boxes[i]->setName(buffer);
	//}

	PxTransform pose = PxTransform(PxVec3(0.f, 0, 0.f), PxQuat(physx::PxHalfPi, PxVec3(0.f, 0.f, 1.f)));

	PxRigidStatic* plane = PxCreateStatic(*m_physics, pose, physx::PxPlaneGeometry(), *m_material);

	const PxU32 numShapes = plane->getNbShapes();
	m_scene->addActor(*plane);

	PxBoxGeometry side1(4.5, 1, .5);
	PxBoxGeometry side2(.5, 1, 4.5);
	pose = PxTransform(PxVec3(0.f, 0.5, 4.f));
	PxRigidStatic* box = PxCreateStatic(*m_physics, pose, side1, *m_material);

	m_scene->addActor(*box);
	m_physXActors.push_back(box);

	pose = PxTransform(PxVec3(0.f, 0.5, -4.f));
	box = PxCreateStatic(*m_physics, pose, side1, *m_material);
	m_scene->addActor(*box);
	m_physXActors.push_back(box);

	pose = PxTransform(PxVec3(4.f, 0.5, 0));
	box = PxCreateStatic(*m_physics, pose, side2, *m_material);
	m_scene->addActor(*box);
	m_physXActors.push_back(box);

	pose = PxTransform(PxVec3(-4.f, 0.5, 0));
	box = PxCreateStatic(*m_physics, pose, side2, *m_material);
	m_scene->addActor(*box);
	m_physXActors.push_back(box);

	//PxParticleSystem* ps;
	////create particle system in PhysX SDK
	////set immutable properties.
	//PxU32 maxParticles = 4000;
	//bool perParticleRestOffset = false;
	//ps = m_physics->createParticleSystem(maxParticles, perParticleRestOffset);
	//ps->setDamping(0.1f);
	//ps->setParticleMass(0.1f);
	//ps->setRestitution(0);
	//ps->setParticleBaseFlag(physx::PxParticleBaseFlag::eCOLLISION_TWOWAY, true);
	//if (ps)
	//{
	//	m_scene->addActor(*ps);
	//	m_particleEmitter = new ParticleEmitter(maxParticles, PxVec3(0, 10, 0), ps, 0.01f);
	//	m_particleEmitter->SetStartVelocityRange(-2.f, 0, -2.f, 2.f, 0.f, 2.f);
	//}

	physx::PxParticleFluid* pf;
	//create particle system in PhysX SDK
	//set immutable properties.
	PxU32 maxParticles = 4000;
	bool perParticleRestOffset = false;
	pf = m_physics->createParticleFluid(maxParticles, perParticleRestOffset);
	pf->setRestParticleDistance(0.5f);
	pf->setDynamicFriction(0.1f);
	pf->setStaticFriction(0.1f);
	pf->setDamping(0.5f);
	pf->setParticleMass(0.1f);
	pf->setRestitution(0);
	//pf->setParticleReadDataFlag(PxParticleReadDataFlag::eDENSITY_BUFFER, true);
	pf->setParticleBaseFlag(physx::PxParticleBaseFlag::eCOLLISION_TWOWAY, true);
	pf->setStiffness(1);
	if (pf)
	{
		m_scene->addActor(*pf);
		m_particleFluidEmitter = new ParticleFluidEmitter(maxParticles, PxVec3(0, 10, 0), pf, 0.01f);
		m_particleFluidEmitter->SetStartVelocityRange(-0.001f, -250.f, -0.001f, 0.001f, -250.f, 0.001f);
	}

	//create some constants for axis of rotation to make definition of quaternions a bit neater
	const physx::PxVec3 X_AXIS = physx::PxVec3(1, 0, 0);
	const physx::PxVec3 Y_AXIS = physx::PxVec3(0, 1, 0);
	const physx::PxVec3 Z_AXIS = physx::PxVec3(0, 0, 1);

	vector<RagdollNode*> ragdollData(16);
	ragdollData[0] = new RagdollNode(PxQuat(PxPi / 2.f, Z_AXIS), NO_PARENT, 1, 3, 1, 1, "lower spine");
	ragdollData[1] = new RagdollNode(PxQuat(PxPi, Z_AXIS), LOWER_SPINE, 1, 1, -1, 1, "left pelvis");
	ragdollData[2] = new RagdollNode(PxQuat(0, Z_AXIS), LOWER_SPINE, 1, 1, -1, 1, "right pelvis");
	ragdollData[3] = new RagdollNode(PxQuat(PxPi / 2.f + 0.2f, Z_AXIS), LEFT_PELVIS, 5, 2, -1, 1, "L upper leg");
	ragdollData[4] = new RagdollNode(PxQuat(PxPi / 2.f - 0.2f, Z_AXIS), RIGHT_PELVIS, 5, 2, -1, 1, "R upper leg");
	ragdollData[5] = new RagdollNode(PxQuat(PxPi / 2.f + 0.2f, Z_AXIS), LEFT_UPPER_LEG, 5, 1.75, -1, 1, "L lower leg");
	ragdollData[6] = new RagdollNode(PxQuat(PxPi / 2.f - 0.2f, Z_AXIS), RIGHT_UPPER_LEG, 5, 1.75, -1, 1, "R lowerleg");
	ragdollData[7] = new RagdollNode(PxQuat(PxPi / 2.f, Z_AXIS), LOWER_SPINE, 1, 3, 1, -1, "upper spine");
	ragdollData[8] = new RagdollNode(PxQuat(PxPi, Z_AXIS), UPPER_SPINE, 1, 1.5, 1, 1, "left clavicle");
	ragdollData[9] = new RagdollNode(PxQuat(0, Z_AXIS), UPPER_SPINE, 1, 1.5, 1, 1, "right clavicle");
	ragdollData[10] = new RagdollNode(PxQuat(PxPi / 2.f, Z_AXIS), UPPER_SPINE, 1, 1, 1, -1, "neck");
	ragdollData[11] = new RagdollNode(PxQuat(PxPi / 2.f, Z_AXIS), NECK, 1, 3, 1, -1, "HEAD");
	ragdollData[12] = new RagdollNode(PxQuat(PxPi - 0.3f, Z_AXIS), LEFT_CLAVICLE, 3, 1.5, -1, 1, "left upper arm");
	ragdollData[13] = new RagdollNode(PxQuat(0.3f, Z_AXIS), RIGHT_CLAVICLE, 3, 1.5, -1, 1, "right upper arm");
	ragdollData[14] = new RagdollNode(PxQuat(PxPi - 0.3f, Z_AXIS), LEFT_UPPER_ARM, 3, 1, -1, 1, "left lower arm");
	ragdollData[15] = new RagdollNode(PxQuat(0.3f, Z_AXIS), RIGHT_UPPER_ARM, 3, 1, -1, 1, "right lower arm");

	physx::PxArticulation* ragdollArticulation = MakeRagdoll(m_physics, ragdollData, PxTransform(PxVec3(5, 5, 5)), 0.1f, m_material);
	m_ragdolls.push_back(ragdollArticulation);
	m_scene->addArticulation(*ragdollArticulation);

	for (unsigned int i = 0; i < ragdollData.size(); i++)
		delete ragdollData[i];
}

bool PhysXApp::Update(float deltaTime)
{
	//close the application if the window closes
	if (glfwWindowShouldClose(m_window) || glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		return false;

	//update the camera's movement
	m_camera->Update(deltaTime);

	PxVec3 rightVec(m_camera->GetTransform()[0].x, m_camera->GetTransform()[0].y, m_camera->GetTransform()[0].z);
	PxVec3 upVec(m_camera->GetTransform()[1].x, m_camera->GetTransform()[1].y, m_camera->GetTransform()[1].z);
	PxVec3 forwardVec(m_camera->GetTransform()[2].x, m_camera->GetTransform()[2].y, m_camera->GetTransform()[2].z);
	PxVec3 cameraPos(m_camera->GetTransform()[3].x, m_camera->GetTransform()[3].y, m_camera->GetTransform()[3].z);

	bool leftMousePressed = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS);

	//PxTransform transform = boxActor->getGlobalPose();

	//printf("%0.2f\t%0.2f\t%0.2f\n", transform.p.x, transform.p.y, transform.p.z);

	if (leftMousePressed && !m_wasLeftMousePressed)
	{
		PxVec3 velocity = forwardVec * -25.f;

		PxShape* shape = m_physics->createShape(PxBoxGeometry(0.25f, 0.25f, 0.25f), *m_material);
		physx::PxRigidDynamic* boxActor = m_physics->createRigidDynamic(PxTransform(cameraPos));
		boxActor->setLinearVelocity(velocity);
		boxActor->attachShape(*shape);
		m_physXActors.push_back(boxActor);
		m_scene->addActor(*boxActor);
		//boxActor->setName("shot box");

		m_wasLeftMousePressed = true;
	}
	else if (!leftMousePressed && m_wasLeftMousePressed)
		m_wasLeftMousePressed = false;

	for (auto artiulcation : m_ragdolls)
	{
		PxU32 numberLinks = artiulcation->getNbLinks();
		vector<PxArticulationLink*> links(numberLinks);
		artiulcation->getLinks(&links[0], numberLinks);

		while (numberLinks--)
		{
			PxArticulationLink* link = links[numberLinks];
			if (!strcmp(link->getName(), "HEAD"))
			{
				if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
					link->addForce(forwardVec * 1000.f);
				if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
					link->addForce(-forwardVec * 1000.f);
				if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS)
					link->addForce(-rightVec * 1000.f);
				if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
					link->addForce(rightVec * 1000.f);
				break;
			}
		}
	}

	m_scene->simulate(deltaTime);
	m_scene->fetchResults(true);

	if (m_particleFluidEmitter && glfwGetKey(m_window, GLFW_KEY_F) == GLFW_PRESS)
		m_particleFluidEmitter->Update(deltaTime);

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
		vector<PxShape*> shapes(numberShapes);
		actor->getShapes(&shapes[0], numberShapes);
		while (numberShapes--)
			AddWidget(shapes[numberShapes], actor);
	}

	for (auto artiulcation : m_ragdolls)
	{
		PxU32 numberLinks = artiulcation->getNbLinks();
		vector<PxArticulationLink*> links(numberLinks);
		artiulcation->getLinks(&links[0], numberLinks);

		while (numberLinks--)
		{
			PxArticulationLink* link = links[numberLinks];
			PxU32 numberShapes = link->getNbShapes();
			vector<PxShape*> shapes(numberShapes);
			link->getShapes(&shapes[0], numberShapes);
			while (numberShapes--)
				AddWidget(shapes[numberShapes], link);
		}
	}

	if (m_particleFluidEmitter)
		m_particleFluidEmitter->RenderParticles();	//render all our particles

	//clear the screen for this frame
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_shaders->Bind();
	
	int loc = m_shaders->GetUniform("projectionView");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(m_camera->GetProjectionView()));
	loc = m_shaders->GetUniform("offset");
	glUniform3fv(loc, 1, value_ptr(vec3(10, 0, 10)));
	
	loc = m_shaders->GetUniform("cameraPosition");
	glUniform3fv(loc, 1, value_ptr(vec3(m_camera->GetTransform()[0].x, m_camera->GetTransform()[1].y, m_camera->GetTransform()[2].z)));
	loc = m_shaders->GetUniform("lightPosition");
	glUniform3fv(loc, 1, value_ptr(vec3(9, 9, 9)));
	
	m_mesh->Draw(GL_TRIANGLES);

	//for (unsigned int i = 0; i < m_FBX->GetMeshCount(); ++i)
	//{
	//	FBXMeshNode* mesh = m_FBX->GetMeshByIndex(i);
	//
	//	unsigned int* glData = (unsigned int*)mesh->m_userData;
	//
	//	glBindVertexArray(glData[0]);
	//	glDrawElements(GL_TRIANGLES, (unsigned int)mesh->m_meshIndices.size(), GL_UNSIGNED_INT, 0);
	//}
	
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
		default:
			break;
	}
}

void PhysXApp::AddSphere(PxShape* shape, PxRigidActor* actor)
{
	physx::PxSphereGeometry geometry;
	float radius = 1;

	bool status = shape->getSphereGeometry(geometry);
	if (status)
		radius = geometry.radius;

	PxMat44 pxm(PxShapeExt::getGlobalPose(*shape, *actor));
	mat4 m(pxm.column0.x, pxm.column0.y, pxm.column0.z, pxm.column0.w, pxm.column1.x, pxm.column1.y, pxm.column1.z, pxm.column1.w, pxm.column2.x, pxm.column2.y, pxm.column2.z, pxm.column2.w, 
			pxm.column3.x, pxm.column3.y, pxm.column3.z, pxm.column3.w);
	vec3 position;

	position.x = pxm.getPosition().x;
	position.y = pxm.getPosition().y;
	position.z = pxm.getPosition().z;
	vec4 colour = vec4(1, 0, 0, 1);
	if (actor->getName() && strcmp(actor->getName(), "Pickup1"))
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

	if (glm::length(cross(vert, normal)) < (1.f / 360.f))
		vert = vec3(0, 1, 0);
	if (glm::length(cross(vert, normal)) < (1.f / 360.f))
		vert = vec3(1, 0, 0);
	
	vec3 vector1 = glm::normalize(cross(normal, vert));
	vec3 vector2 = cross(vector1, normal);

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
	physx::PxPlaneGeometry geometry;

	PxMat44 pxm(PxShapeExt::getGlobalPose(*shape, *actor));
	mat4 m(pxm.column0.x, pxm.column0.y, pxm.column0.z, pxm.column0.w, pxm.column1.x, pxm.column1.y, pxm.column1.z, pxm.column1.w, pxm.column2.x, pxm.column2.y, pxm.column2.z, pxm.column2.w, 
			pxm.column3.x, pxm.column3.y, pxm.column3.z, pxm.column3.w);
	vec3 position;
	position.x = pxm.getPosition().x;
	position.y = pxm.getPosition().y;
	position.z = pxm.getPosition().z;

	vec3 normal(0);

	bool status = shape->getPlaneGeometry(geometry);
	if (status)
	{
		physx::PxPlane plane = PxPlaneEquationFromTransform(PxTransform(pxm));
		normal = vec3(plane.n.x, plane.n.y, plane.n.z);
	}

	vec4 colour = vec4(1, 0, 0, 1);
	if (actor->getName() && strcmp(actor->getName(), "Pickup1"))
		colour = vec4(0, 1, 0, 1);

	DrawPlane(position, normal, colour);
}

void PhysXApp::AddCapsule(PxShape* shape, PxRigidActor* actor)
{
	physx::PxCapsuleGeometry geometry;
	float radius = 1.f;
	float halfHeight = 2.f;

	bool status = shape->getCapsuleGeometry(geometry);
	if (status)
	{
		radius = geometry.radius;
		halfHeight = geometry.halfHeight;
	}

	PxMat44 pxm(PxShapeExt::getGlobalPose(*shape, *actor));
	mat4 m(pxm.column0.x, pxm.column0.y, pxm.column0.z, pxm.column0.w, pxm.column1.x, pxm.column1.y, pxm.column1.z, pxm.column1.w, pxm.column2.x, pxm.column2.y, pxm.column2.z, pxm.column2.w, 
			pxm.column3.x, pxm.column3.y, pxm.column3.z, pxm.column3.w);
	vec3 position;

	position.x = pxm.getPosition().x;
	position.y = pxm.getPosition().y;
	position.z = pxm.getPosition().z;
	vec4 colour = vec4(1, 0, 0, 1);
	if (actor->getName() && strcmp(actor->getName(), "Pickup1"))
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
	mat4 m(pxm.column0.x, pxm.column0.y, pxm.column0.z, pxm.column0.w, pxm.column1.x, pxm.column1.y, pxm.column1.z, pxm.column1.w, pxm.column2.x, pxm.column2.y, pxm.column2.z, pxm.column2.w, 
			pxm.column3.x, pxm.column3.y, pxm.column3.z, pxm.column3.w);
	vec3 position;

	position.x = pxm.getPosition().x;
	position.y = pxm.getPosition().y;
	position.z = pxm.getPosition().z;
	vec3 extents = vec3(width, height, length);
	vec4 colour = vec4(1, 0, 0, 1);
	if (actor->getName() && strcmp(actor->getName(), "Pickup1"))
		colour = vec4(0, 1, 0, 1);

	Gizmos::AddAABBFilled(position, extents, colour, &m);
}

//void PhysXApp::CreateOpenGLBuffers()
//{
//	//create the GL VAO/VBO/IBO data for each mesh
//	for (unsigned int i = 0; i < m_FBX->GetMeshCount(); ++i)
//	{
//		FBXMeshNode* mesh = m_FBX->GetMeshByIndex(i);
//
//		//storage for the opengl data in 3 unsigned int
//		unsigned int* glData = new unsigned int[3];
//
//		glGenVertexArrays(1, &glData[0]);
//		glBindVertexArray(glData[0]);
//
//		glGenBuffers(1, &glData[1]);
//		glGenBuffers(1, &glData[2]);
//
//		glBindBuffer(GL_ARRAY_BUFFER, glData[1]);
//		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glData[2]);
//
//		glBufferData(GL_ARRAY_BUFFER, mesh->m_meshVertices.size() * sizeof(FBXVertex), mesh->m_meshVertices.data(), GL_STATIC_DRAW);
//		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->m_meshIndices.size() * sizeof(unsigned int), mesh->m_meshIndices.data(), GL_STATIC_DRAW);
//
//		glEnableVertexAttribArray(0); //position
//		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (void*)FBXVertex::PositionOffset);
//		glEnableVertexAttribArray(1); //colour
//		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (void*)FBXVertex::ColourOffset);
//		glEnableVertexAttribArray(2); //normal
//		glVertexAttribPointer(2, 4, GL_FLOAT, GL_TRUE, sizeof(FBXVertex), (void*)FBXVertex::NormalOffset);
//		glEnableVertexAttribArray(3); //tangents
//		glVertexAttribPointer(3, 4, GL_FLOAT, GL_TRUE, sizeof(FBXVertex), (void*)FBXVertex::TangentOffset);
//		glEnableVertexAttribArray(4); //tex coords
//		glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (void*)FBXVertex::TexCoord1Offset);
//
//		glBindVertexArray(0);
//		glBindBuffer(GL_ARRAY_BUFFER, 0);
//		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
//
//		mesh->m_userData = glData;
//	}
//}
//
//void PhysXApp::CleanupOpenGLBuffers()
//{
//	//clean up the vertex data attached to each mesh
//	for (unsigned int i = 0; i < m_FBX->GetMeshCount(); ++i)
//	{
//		FBXMeshNode* mesh = m_FBX->GetMeshByIndex(i);
//
//		unsigned int* glData = (unsigned int*)mesh->m_userData;
//
//		glDeleteVertexArrays(1, &glData[0]);
//		glDeleteBuffers(1, &glData[1]);
//		glDeleteBuffers(1, &glData[2]);
//
//		delete[] mesh->m_userData;
//	}
//}

void PhysXApp::CreateShader()
{
	m_shaders->LoadShader(GL_VERTEX_SHADER, "./res/OBJVertex.vert");
	m_shaders->LoadShader(GL_FRAGMENT_SHADER, "./res/OBJFragment.frag");
	m_shaders->Link();
}

//void PhysXApp::AttachedRigidBodyConvex(float density, PxMaterial* physicsMaterial, PxRigidActor* actor)
//{
//	//need a placeholder box
//	PxBoxGeometry box = PxBoxGeometry(0.25f, 0.25f, 0.25f);
//	PxTransform transform(*(PxMat44*)(&m_camera->GetTransform()[0]));	//PhysX and GLM matricies are the same internally so we simply cast between them
//	actor = PxCreateDynamic(*m_physics, transform, box, *physicsMaterial, density);
//	actor->userData = m_FBX;	//link the PhysX actor to our FBX model
//
//	int numberVerts = 0;
//	//find out how many verts there are in total in tank model
//	for (unsigned int i = 0; i < m_FBX->GetMeshCount(); ++i)
//	{
//		FBXMeshNode* mesh = m_FBX->GetMeshByIndex(i);
//		numberVerts += mesh->m_meshVertices.size();
//	}
//	//reserve space for vert buffer
//	PxVec3* verts = new PxVec3[numberVerts];	//temporary buffer for our verts
//	int vertIDX = 0;
//
//	//copy our verts from all the sub meshes and tranform them into the same space
//	for (unsigned int i = 0; i < m_FBX->GetMeshCount(); ++i)
//	{
//		FBXMeshNode* mesh = m_FBX->GetMeshByIndex(i);
//		numberVerts = mesh->m_meshVertices.size();
//		for (int vertCount = 0; vertCount < numberVerts; vertCount++)
//		{
//			vec4 temp = mesh->m_globalTransform * mesh->m_meshVertices[vertCount].m_position;
//			verts[vertIDX++] = PxVec3(temp.x, temp.y, temp.z);
//		}
//	}
//	PxConvexMeshDesc convexDesc;
//	convexDesc.points.count = numberVerts;
//	convexDesc.points.stride = sizeof(PxVec3);
//	convexDesc.points.data = verts;
//	convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
//	convexDesc.vertexLimit = 128;
//	PxDefaultMemoryOutputStream* buf = new PxDefaultMemoryOutputStream();
//	assert(m_cooker->cookConvexMesh(convexDesc, *buf));
//	PxU8* contents = buf->getData();
//	PxU32 size = buf->getSize();
//	PxDefaultMemoryInputData input(contents, size);
//	PxConvexMesh* convexMesh = m_physics->createConvexMesh(input);
//	PxTransform pose = PxTransform(PxVec3(10.f, 1, 10.f));
//	PxShape* convexShape = actor->createShape(PxConvexMeshGeometry(convexMesh), *physicsMaterial, pose);
//	//remove the placeholder box we started with
//	int numberShapes = actor->getNbShapes();
//	vector<PxShape*> shapes(numberShapes);
//	actor->getShapes(&shapes[0], numberShapes);
//	actor->detachShape(*shapes[0]);
//	delete(verts);	//delete our temporary vert buffer.
//	//Add it to the scene
//	m_scene->addActor(*actor);
//}
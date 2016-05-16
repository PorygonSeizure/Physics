#include "PhysicsApp.h"
#include "gl_core_4_4.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <Camera.h>
#include "Gizmos.h"
#include "Render.h"

using glm::vec3;
using glm::vec4;
using physx::PxShape;
using physx::PxRigidActor;
using physx::PxGeometryType;
using physx::PxActor;
using physx::PxArticulationLink;

#define Assert(val) if (val){}else{ *((char*)0) = 0;}
#define ArrayCount(val) (sizeof(val)/sizeof(val[0]))

void FormatDebugOutputARB(char outStr[], size_t outStrSize, GLenum source, GLenum type, GLuint id, GLenum severity, const char* msg);

void __stdcall DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam);

bool PhysicsApp::Startup()
{
	CreateGLFWWindow("AIE OpenGL Application", 1280, 720);

#ifdef _DEBUG
    glDebugMessageCallback((GLDEBUGPROC)DebugCallback, stderr);	// print debug output to stderr
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

	glfwSwapInterval(1);

	//start the gizmo system that can draw basic shapes
	Gizmos::Create();

	//create a camera
	m_camera = new FlyCamera(glm::pi<float>() * 0.25f, 16.f / 9.f, 0.1f, 1000.f);
	m_camera->SetLookAtFrom(vec3(10, 10, 10), vec3(0));

	m_renderer = new Renderer();

	return true;
}

void PhysicsApp::Shutdown()
{
	//delete our camera, renderer and cleanup gizmos
	delete m_camera;
	delete m_renderer;
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

	//clear the gizmos out for this frame
	Gizmos::Clear();

	//////////////////////////////////////////////////////////////////////////
	// YOUR UPDATE CODE HERE
	//////////////////////////////////////////////////////////////////////////

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

	//return true, else the application closes
	return true;
}

void PhysicsApp::Draw()
{
	//clear the screen for this frame
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);

	//...for now let's add a grid to the gizmos
	for (int i = 0; i < 21; ++i)
	{
		Gizmos::AddLine(vec3(-10 + i, 0, 10), vec3(-10 + i, 0, -10), i == 10 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));

		Gizmos::AddLine(vec3(10, 0, -10 + i), vec3(-10, 0, -10 + i), i == 10 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));
	}

	//////////////////////////////////////////////////////////////////////////
	// DRAW YOUR THINGS HERE
	//////////////////////////////////////////////////////////////////////////

	//display the 3D gizmos
	Gizmos::Draw(m_camera->GetProjectionView());
	m_renderer->RenderAndClear(m_camera->GetProjectionView());

	// get a orthographic projection matrix and draw 2D gizmos
	int width = 0;
	int height = 0;
	glfwGetWindowSize(m_window, &width, &height);
	glm::mat4 guiMatrix = glm::ortho<float>(0, (float)width, 0, (float)height);

	Gizmos::Draw2D(guiMatrix);
}

void AddWidget(PxShape* shape, PxRigidActor* actor, vec4 geoColor)
{
	physx::PxTransform fullTransform = physx::PxShapeExt::getGlobalPose(*shape, *actor);
	vec3 actorPosition(fullTransform.p.x, fullTransform.p.y, fullTransform.p.z);
	glm::quat actorRotation(fullTransform.q.w, fullTransform.q.x, fullTransform.q.y, fullTransform.q.z);
	glm::mat4 rot(actorRotation);

	mat4 rotateMatrix = glm::rotate(10.f, vec3(7, 7, 7));

	PxGeometryType::Enum geoType = shape->getGeometryType();

	switch (geoType)
	{
	case (PxGeometryType::eBOX) :
	{
		physx::PxBoxGeometry geo;
		shape->getBoxGeometry(geo);
		vec3 extents(geo.halfExtents.x, geo.halfExtents.y, geo.halfExtents.z);
		Gizmos::AddAABBFilled(actorPosition, extents, geoColor, &rot);
		break;
	}
	case (PxGeometryType::eCAPSULE) :
	{
		physx::PxCapsuleGeometry geo;
		shape->getCapsuleGeometry(geo);
		Gizmos::AddCapsule(actorPosition, geo.halfHeight * 2, geo.radius, 16, 16, geoColor, &rot);
		break;
	}
	case (PxGeometryType::eSPHERE) :
	{
		physx::PxSphereGeometry geo;
		shape->getSphereGeometry(geo);
		Gizmos::AddSphere(actorPosition, geo.radius, 16, 16, geoColor, &rot);
		break;
	}
	case (PxGeometryType::ePLANE) :
		break;
	}
}

void PhysicsApp::RenderGizmos(physx::PxScene* physicsScene)
{
	physx::PxActorTypeFlags desiredTypes = physx::PxActorTypeFlag::eRIGID_STATIC | physx::PxActorTypeFlag::eRIGID_DYNAMIC;
	physx::PxU32 actorCount = physicsScene->getNbActors(desiredTypes);
	PxActor** actorList = new PxActor*[actorCount];
	physicsScene->getActors(desiredTypes, actorList, actorCount);

	vec4 geoColor(1, 0, 0, 1);
	for (int actorIndex = 0; actorIndex < (int)actorCount; ++actorIndex)
	{
		PxActor* currActor = actorList[actorIndex];
		if (currActor->isRigidActor())
		{
			PxRigidActor* rigidActor = (PxRigidActor*)currActor;
			physx::PxU32 shapeCount = rigidActor->getNbShapes();
			PxShape** shapes = new PxShape*[shapeCount];
			rigidActor->getShapes(shapes, shapeCount);

			for (int shapeIndex = 0; shapeIndex < (int)shapeCount; ++shapeIndex)
			{
				PxShape* currShape = shapes[shapeIndex];
				AddWidget(currShape, rigidActor, geoColor);
			}

			delete[]shapes;
		}
	}

	delete[] actorList;

	int articulationCount = physicsScene->getNbArticulations();

	for (int a = 0; a < articulationCount; ++a)
	{
		physx::PxArticulation* articulation;
		physicsScene->getArticulations(&articulation, 1, a);

		int linkCount = articulation->getNbLinks();

		PxArticulationLink** links = new PxArticulationLink*[linkCount];
		articulation->getLinks(links, linkCount);

		for (int l = 0; l < linkCount; ++l)
		{
			PxArticulationLink* link = links[l];
			int shapeCount = link->getNbShapes();

			for (int s = 0; s < shapeCount; ++s)
			{
				PxShape* shape;
				link->getShapes(&shape, 1, s);
				AddWidget(shape, link, geoColor);
			}
		}
		delete[] links;
	}
}

#if _DEBUG

void FormatDebugOutputARB(char outStr[], size_t outStrSize, GLenum source, GLenum type, GLuint id, GLenum severity, const char* msg)
{
    char sourceStr[512];
    const char* sourceFmt = "UNDEFINED(0x%04X)";
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
		sourceFmt = "API";
		break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		sourceFmt = "WINDOW_SYSTEM";
		break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
		sourceFmt = "SHADER_COMPILER";
		break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
		sourceFmt = "THIRD_PARTY";
		break;
    case GL_DEBUG_SOURCE_APPLICATION:
		sourceFmt = "APPLICATION";
		break;
    case GL_DEBUG_SOURCE_OTHER:
		sourceFmt = "OTHER";
		break;
    }

    _snprintf_s(sourceStr, 256, sourceFmt, source);

    char typeStr[512];
    const char* typeFmt = "UNDEFINED(0x%04X)";
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
		typeFmt = "ERROR";
		break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		typeFmt = "DEPRECATED_BEHAVIOR";
		break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		typeFmt = "UNDEFINED_BEHAVIOR";
		break;
    case GL_DEBUG_TYPE_PORTABILITY:
		typeFmt = "PORTABILITY";
		break;
    case GL_DEBUG_TYPE_PERFORMANCE:
		typeFmt = "PERFORMANCE";
		break;
    case GL_DEBUG_TYPE_OTHER:
		typeFmt = "OTHER";
		break;
    }
    _snprintf(typeStr, 256, typeFmt, type);


    char severityStr[512];
    const char* severityFmt = "UNDEFINED";
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
		severityFmt = "HIGH";
		break;
    case GL_DEBUG_SEVERITY_MEDIUM:
		severityFmt = "MEDIUM";
		break;
    case GL_DEBUG_SEVERITY_LOW:
		severityFmt = "LOW";
		break;
    }

    _snprintf(severityStr, 256, severityFmt, severity);

    _snprintf(outStr, outStrSize, "OpenGL: %s [source=%s type=%s severity=%s id=%d]", msg, sourceStr, typeStr, severityStr, id);
}

void __stdcall DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
    (void)length;
    FILE* outFile = (FILE*)userParam;
    char finalMessage[512];
    FormatDebugOutputARB(finalMessage, 256, source, type, id, severity, message);

    if (type != GL_DEBUG_TYPE_OTHER)
        fprintf(outFile, "%s\n", finalMessage);
}

#endif
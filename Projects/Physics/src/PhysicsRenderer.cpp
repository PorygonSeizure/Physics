#include "PhysicsRenderer.h"
#include "PhysicsScene.h"
#include "PhysicsObject.h"
#include <Gizmos.h>
#include <glm/glm.hpp>

using namespace Physics;

void PhysicsRenderer::Render(PhysicsScene* scene)
{
	auto& objects = scene->GetPhysicsObject();
	for (auto iter = objects.begin(); iter != objects.end(); iter++)
	{
		auto obj = (*iter);

		RenderInfo* info = GetRenderInfo(obj);
		glm::vec4 color = info->color;

		obj->MakeGizmo(color);
	}
}

PhysicsRenderer::RenderInfo* PhysicsRenderer::GetRenderInfo(PhysicsObject* object) { return &m_renderInfo[object]; }
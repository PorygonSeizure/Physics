#ifndef _PHYSICS_RENDERER_H_
#define _PHYSICS_RENDERER_H_

#include <unordered_map>
#include <glm/glm.hpp>

namespace Physics
{
class PhysicsScene;
class PhysicsObject;

class PhysicsRenderer
{
public:
	PhysicsRenderer() {}
	virtual ~PhysicsRenderer() {}

	struct RenderInfo
	{
		RenderInfo() :color(1.f, 1.f, 1.f, 1.f) {}
		glm::vec4 color;
	};

	void Render(PhysicsScene* scene);

	RenderInfo* GetRenderInfo(PhysicsObject* object);

protected:
	std::unordered_map<PhysicsObject*, RenderInfo> m_renderInfo;
};
}

#endif
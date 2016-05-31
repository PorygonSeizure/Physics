#ifndef _PHYSICS_SCENE_H_
#define _PHYSICS_SCENE_H_

#include <vector>
#include <type_traits>
#include <glm/glm.hpp>

namespace Physics
{
class PhysicsObject;
class RigidBody;
struct IntersectData
{
	PhysicsObject* obj1;
	PhysicsObject* obj2;
	float intersection;
	float bounciness;
	float velAlongNormal;
	glm::vec3 collisionNorm;
	glm::vec3 relativeVelo;
	glm::vec3 collisionVec;
	glm::vec3 forceVec;
};

class PhysicsScene
{
public:
	PhysicsScene() {}
	virtual ~PhysicsScene();

	void Simulate(glm::vec3 gravity, float deltaTime);

	template<typename T, typename... TArgs>
	T* CreatePhysicsObject(TArgs... args)
	{
		static_assert(std::is_base_of<PhysicsObject, T>::value, "CreatePhysicsObject T Must Inherit from RigidBody");

		T* object = new T(args...);
		m_physicsObjects.push_back(object);
		return object;
	}

	void DestroyPhysicsObject(PhysicsObject* object);

	inline const std::vector<PhysicsObject*>& GetPhysicsObject() const { return m_physicsObjects; }

	void CheckForCollision();
	void ResolveCollisions();

	//bool Plane2Plane(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data);
	//bool Plane2Sphere(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data);
	//bool Plane2Box(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data);
	//bool Sphere2Plane (PhysicsObject* object1, PhysicsObject* object2, IntersectData* data);
	bool Sphere2Sphere(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data);
	bool Sphere2Box(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data);
	//bool Box2Plane(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data);
	bool Box2Sphere(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data);
	bool Box2Box(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data);

protected:
	std::vector<PhysicsObject*> m_physicsObjects;
	std::vector<IntersectData> m_collisions;
};
}


#endif
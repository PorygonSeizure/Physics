#include "PhysicsScene.h"
#include "PhysicsObject.h"
#include "Plane.h"
#include "Sphere.h"
#include "Box.h"

using namespace Physics;
using glm::vec3;

void PhysicsScene::Simulate(vec3 gravity, float deltaTime)
{
	for (auto iter = m_physicsObjects.begin(); iter != m_physicsObjects.end(); iter++)
	{
		PhysicsObject* obj = (*iter);
		obj->Update(gravity, deltaTime);
	}
}

void PhysicsScene::DestroyPhysicsObject(PhysicsObject* object)
{
	//find the object in the collection and remove it
	auto iter = std::find(m_physicsObjects.begin(), m_physicsObjects.end(), object);
	if (iter != m_physicsObjects.end())
		m_physicsObjects.erase(iter);

	//free the memory
	delete object;
}

//typedef bool(*fn)(PhysicsObject*, PhysicsObject*);
//static fn collisionFunctionArray[] =
//{
//	PhysicsScene::Plane2Plane,
//	PhysicsScene::Plane2Sphere,
//	PhysicsScene::Plane2Box,
//	PhysicsScene::Sphere2Plane,
//	PhysicsScene::Sphere2Sphere,
//	PhysicsScene::Sphere2Box,
//	PhysicsScene::Box2Plane,
//	PhysicsScene::Box2Sphere,
//	PhysicsScene::Box2Box
//};

void PhysicsScene::CheckForCollision()
{
	//int objectsCount = m_physicsObjects.size();
	//
	//for (int outer = 0; outer < objectsCount - 1; outer++)
	//{
	//	for (int inner = outer + 1; inner < objectsCount; inner++)
	//	{
	//		PhysicsObject* object1 = m_physicsObjects[outer];
	//		PhysicsObject* object2 = m_physicsObjects[inner];
	//
	//		int functionIndex = (object1->GetShapeID() * 3) + object2->GetShapeID();
	//		fn collisionFunctionPtr = collisionFunctionArray[functionIndex];
	//		if (collisionFunctionPtr != NULL)
	//			collisionFunctionPtr(object1, object2);
	//	}
	//}

	bool collision = false;
	
	//for (int outer = 0; outer < objectsCount - 1; outer++)
	for (auto iter0 = m_physicsObjects.begin(); iter0 != m_physicsObjects.end(); iter0++)
	{
		//for (int inner = outer + 1; inner < objectsCount; inner++)
		for (auto iter1 = iter0 + 1; iter1 != m_physicsObjects.end(); iter1++)
		{
			PhysicsObject* object1 = (*iter0);
			PhysicsObject* object2 = (*iter1);
			
			IntersectionData data;

			switch (object1->GetShapeID())
			{
			case PLANE:
				switch (object2->GetShapeID())
				{
				case PLANE:
					collision = Plane2Plane(object1, object2, data);
				case SPHERE:
					collision = Plane2Sphere(object1, object2, data);
				case BOX:
					collision = Plane2Box(object1, object2, data);
				}
			case SPHERE:
				switch (object2->GetShapeID())
				{
				case PLANE:
					collision = Sphere2Plane(object1, object2, data);
				case SPHERE:
					collision = Sphere2Sphere(object1, object2, data);
				case BOX:
					collision = Sphere2Box(object1, object2, data);
				}
			case BOX:
				switch (object2->GetShapeID())
				{
				case PLANE:
					collision = Box2Plane(object1, object2, data);
				case SPHERE:
					collision = Box2Sphere(object1, object2, data);
				case BOX:
					collision = Box2Box(object1, object2, data);
				}
			}

			if (collision)
			{
				collision = false;
			}
		}
	}
}

bool PhysicsScene::Plane2Plane(PhysicsObject* object1, PhysicsObject* object2, IntersectionData& data)
{
	Plane* plane1 = dynamic_cast<Plane*>(object1);
	Plane* plane2 = dynamic_cast<Plane*>(object2);
	if (plane1 != NULL && plane2 != NULL)
	{
		return (plane1->m_normal != plane2->m_normal || plane1->m_normal != -plane2->m_normal);
	}
	return false;
}

bool PhysicsScene::Plane2Sphere(PhysicsObject* object1, PhysicsObject* object2, IntersectionData& data)
{
	Plane* plane1 = dynamic_cast<Plane*>(object1);
	Sphere* sphere2 = dynamic_cast<Sphere*>(object2);
	if (plane1 != NULL && sphere2 != NULL)
	{
		vec3 temp(sphere2->GetPosition() - vec3(0.f, plane1->m_distanceToOrigin, 0.f));
		return (glm::dot(plane1->m_normal, temp) <= sphere2->GetRadius());
	}
	return false;
}

bool PhysicsScene::Plane2Box(PhysicsObject* object1, PhysicsObject* object2, IntersectionData& data)
{
	Plane* plane1 = dynamic_cast<Plane*>(object1);
	Box* box2 = dynamic_cast<Box*>(object2);
	if (plane1 != NULL && box2 != NULL)
	{
		float temp1 = glm::dot(plane1->m_normal, (box2->GetPosition() - vec3(0.f, plane1->m_distanceToOrigin, 0.f)));
		float temp2 = ((box2->m_length/ 2.f) * glm::abs(glm::dot(plane1->m_normal, vec3(1.f, 0.f, 0.f))));
		temp2 += ((box2->m_height / 2.f) * glm::abs(glm::dot(plane1->m_normal, vec3(0.f, 1.f, 0.f))));
		temp2 += ((box2->m_width / 2.f) * glm::abs(glm::dot(plane1->m_normal, vec3(0.f, 0.f, 1.f))));
		return (temp1 <= temp2);
	}
	return false;
}

bool PhysicsScene::Sphere2Plane(PhysicsObject* object1, PhysicsObject* object2, IntersectionData& data)
{
	Sphere* sphere1 = dynamic_cast<Sphere*>(object1);
	Plane* plane2 = dynamic_cast<Plane*>(object2);
	if (sphere1 != NULL && plane2 != NULL)
	{
		vec3 temp(sphere1->GetPosition() - vec3(0.f, plane2->m_distanceToOrigin, 0.f));
		return (glm::dot(plane2->m_normal, temp) <= sphere1->GetRadius());
	}
	return false;
}

bool PhysicsScene::Sphere2Sphere(PhysicsObject* object1, PhysicsObject* object2, IntersectionData& data)
{
	Sphere* sphere1 = dynamic_cast<Sphere*>(object1);
	Sphere* sphere2 = dynamic_cast<Sphere*>(object2);
	if (sphere1 != NULL && sphere2 != NULL)
	{
		float temp1 = glm::distance(sphere1->GetPosition(), sphere2->GetPosition());
		float temp2 = (sphere1->GetRadius() + sphere2->GetRadius());
		//temp2 *= temp2;
		return (temp1 <= temp2);
	}
	return false;
}

bool PhysicsScene::Sphere2Box(PhysicsObject* object1, PhysicsObject* object2, IntersectionData& data)
{
	Sphere* sphere1 = dynamic_cast<Sphere*>(object1);
	Box* box2 = dynamic_cast<Box*>(object2);
	if (sphere1 != NULL && box2 != NULL)
	{
		vec3 pc(glm::clamp(sphere1->GetPosition(), box2->m_minVert, box2->m_maxVert));
		vec3 temp(sphere1->GetPosition() - pc);
		return (glm::dot(temp, temp) <= (sphere1->GetRadius() * sphere1->GetRadius()));
	}
	return false;
}

bool PhysicsScene::Box2Plane(PhysicsObject* object1, PhysicsObject* object2, IntersectionData& data)
{
	Box* box1 = dynamic_cast<Box*>(object1);
	Plane* plane2 = dynamic_cast<Plane*>(object2);
	if (box1 != NULL && plane2 != NULL)
	{
		float temp1 = glm::dot(plane2->m_normal, (box1->GetPosition() - vec3(0.f, plane2->m_distanceToOrigin, 0.f)));
		float temp2 = ((box1->m_length / 2.f) * glm::abs(glm::dot(plane2->m_normal, vec3(1.f, 0.f, 0.f))));
		temp2 += ((box1->m_height / 2.f) * glm::abs(glm::dot(plane2->m_normal, vec3(0.f, 1.f, 0.f))));
		temp2 += ((box1->m_width / 2.f) * glm::abs(glm::dot(plane2->m_normal, vec3(0.f, 0.f, 1.f))));
		return (temp1 <= temp2);
	}
	return false;
}

bool PhysicsScene::Box2Sphere(PhysicsObject* object1, PhysicsObject* object2, IntersectionData& data)
{
	Box* box1 = dynamic_cast<Box*>(object1);
	Sphere* sphere2 = dynamic_cast<Sphere*>(object2);
	if (box1 != NULL && sphere2 != NULL)
	{
		vec3 pc(glm::clamp(sphere2->GetPosition(), box1->m_minVert, box1->m_maxVert));
		vec3 temp(sphere2->GetPosition() - pc);
		return (glm::dot(temp, temp) <= (sphere2->GetRadius() * sphere2->GetRadius()));
	}
	return false;
}

bool PhysicsScene::Box2Box(PhysicsObject* object1, PhysicsObject* object2, IntersectionData& data)
{
	Box* box1 = dynamic_cast<Box*>(object1);
	Box* box2 = dynamic_cast<Box*>(object2);
	if (box1 != NULL && box2 != NULL)
	{
		bool temp1 = (box1->m_maxVert.x >= box2->m_minVert.x && box2->m_maxVert.x >= box1->m_minVert.x);
		bool temp2 = (box1->m_maxVert.y >= box2->m_minVert.y && box2->m_maxVert.y >= box1->m_minVert.y);
		bool temp3 = (box1->m_maxVert.z >= box2->m_minVert.z && box2->m_maxVert.z >= box1->m_minVert.z);
		return (temp1 && temp2 && temp3);
	}
	return false;
}
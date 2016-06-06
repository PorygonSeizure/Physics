#include "PhysicsScene.h"
#include "PhysicsObject.h"
#include "RigidBody.h"
#include "Plane.h"
#include "Sphere.h"
#include "Box.h"

using namespace Physics;
using glm::vec3;
using glm::dot;
using glm::normalize;
using glm::min;

PhysicsScene::~PhysicsScene()
{
	for (unsigned int i = 0; i < m_physicsObjects.size(); i++)
		delete m_physicsObjects[i];
}

void PhysicsScene::Simulate(vec3 gravity, float deltaTime)
{
	for (auto iter = m_physicsObjects.begin(); iter != m_physicsObjects.end(); iter++)
	{
		(*iter)->Update(gravity, deltaTime);

		if ((*iter)->GetShapeID() != PLANE && (*iter)->GetShapeID() != JOINT)
		{
			RigidBody* obj = dynamic_cast<RigidBody*>(*iter);

			const vec3& pos = obj->GetPosition();
			const vec3& vel = obj->GetVelocity();
			if (pos.y < 0)
			{
				obj->SetPosition(vec3(pos.x, 0.f, pos.z));
				obj->SetVelocity(vec3(vel.x, (-vel.y * obj->GetBouciness()), vel.z));
			}
		}
	}

	CheckForCollision();
	ResolveCollisions();
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
	//		if (collisionFunctionPtr)
	//			collisionFunctionPtr(object1, object2);
	//	}
	//}

	m_collisions.clear();
	bool collision = false;
	
	//for (int outer = 0; outer < objectsCount - 1; outer++)
	for (auto iter0 = m_physicsObjects.begin(); iter0 != m_physicsObjects.end(); iter0++)
	{
		//for (int inner = outer + 1; inner < objectsCount; inner++)
		for (auto iter1 = iter0 + 1; iter1 != m_physicsObjects.end(); iter1++)
		{
			PhysicsObject* object1 = (*iter0);
			PhysicsObject* object2 = (*iter1);
			
			IntersectData data;
			data.obj1 = object1;
			data.obj2 = object2;

			switch (object1->GetShapeID())
			{
			//case PLANE:
			//	switch (object2->GetShapeID())
			//	{
			//	case PLANE:
			//		collision = Plane2Plane(object1, object2, &data);
			//		break;
			//	case SPHERE:
			//		collision = Plane2Sphere(object1, object2, &data);
			//		break;
			//	case BOX:
			//		collision = Plane2Box(object1, object2, &data);
			//		break;
			//	}
			//	break;
			case SPHERE:
				switch (object2->GetShapeID())
				{
				//case PLANE:
				//	collision = Sphere2Plane(object1, object2, &data);
				//	break;
				case SPHERE:
					collision = Sphere2Sphere(object1, object2, &data);
					break;
				case BOX:
					collision = Sphere2Box(object1, object2, &data);
					break;
				}
				break;
			case BOX:
				switch (object2->GetShapeID())
				{
				//case PLANE:
				//	collision = Box2Plane(object1, object2, &data);
				//	break;
				case SPHERE:
					collision = Box2Sphere(object1, object2, &data);
					break;
				case BOX:
					collision = Box2Box(object1, object2, &data);
					break;
				}
				break;
			}

			if (collision)
			{
				m_collisions.push_back(data);
				collision = false;
			}
		}
	}
}

void PhysicsScene::ResolveCollisions()
{
	for (auto iter0 = m_collisions.begin(); iter0 != m_collisions.end(); iter0++)
	{
		IntersectData temp = (*iter0);
		for (auto iter1 = m_physicsObjects.begin(); iter1 != m_physicsObjects.end(); iter1++)
		{
			if ((*iter0).obj1 == (*iter1))
			{
				for (auto iter2 = iter1 + 1; iter2 != m_physicsObjects.end(); iter2++)
				{
					if ((*iter0).obj2 == (*iter2))
					{
						if ((*iter1)->GetShapeID() == JOINT || (*iter2)->GetShapeID() == JOINT)
						{

						}
						if ((*iter1)->GetShapeID() == PLANE && (*iter2)->GetShapeID() == SPHERE)
						{
							Sphere* obj2 = dynamic_cast<Sphere*>(*iter2);
							obj2->ApplyForce(2.f * temp.forceVec);
							obj2->SetPosition(obj2->GetPosition() + (temp.collisionNorm * temp.intersection * 0.5f));
						}
						else
						{
							//if (temp.velAlongNormal < 0)
							//	continue;

							RigidBody* obj1 = dynamic_cast<RigidBody*>(*iter1);
							RigidBody* obj2 = dynamic_cast<RigidBody*>(*iter2);

							//obj1->SetVelocity(obj1->GetVelocity() - (obj1->GetInverseMass() * temp.forceVec));
							//obj2->SetVelocity(obj2->GetVelocity() + (obj2->GetInverseMass() * temp.forceVec));

							obj1->ApplyForceToActor(obj2, 2.f * temp.forceVec);
							vec3 seperationVec = temp.collisionNorm * temp.intersection * 0.5f;

							obj1->SetPosition(obj1->GetPosition() - seperationVec);
							obj2->SetPosition(obj2->GetPosition() + seperationVec);
						}
					}
				}
			}
		}
	}
}

//bool PhysicsScene::Plane2Plane(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data)
//{
//	Plane* plane1 = dynamic_cast<Plane*>(object1);
//	Plane* plane2 = dynamic_cast<Plane*>(object2);
//	if (plane1 && plane2)
//	{
//		return (plane1->m_normal != plane2->m_normal || plane1->m_normal != -plane2->m_normal);
//	}
//	return false;
//}

//bool PhysicsScene::Plane2Sphere(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data)
//{
//	Plane* plane = dynamic_cast<Plane*>(object1);
//	Sphere* sphere = dynamic_cast<Sphere*>(object2);
//	if (plane && sphere)
//	{
//		vec3 temp(sphere->GetPosition() - vec3(0.f, plane->m_distanceToOrigin, 0.f));
//		return (dot(plane->m_normal, temp) < sphere->GetRadius());
//	}
//	return false;
//}

//bool PhysicsScene::Plane2Box(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data)
//{
//	Plane* plane = dynamic_cast<Plane*>(object1);
//	Box* box = dynamic_cast<Box*>(object2);
//	if (plane && box)
//	{
//		float temp1 = dot(plane->m_normal, (box->GetPosition() - vec3(0.f, plane->m_distanceToOrigin, 0.f)));
//		float temp2 = ((box->m_length/ 2.f) * glm::abs(dot(plane->m_normal, vec3(1.f, 0.f, 0.f))));
//		temp2 += ((box->m_height / 2.f) * glm::abs(dot(plane->m_normal, vec3(0.f, 1.f, 0.f))));
//		temp2 += ((box->m_width / 2.f) * glm::abs(dot(plane->m_normal, vec3(0.f, 0.f, 1.f))));
//		return (temp1 <= temp2);
//	}
//	return false;
//}

//bool PhysicsScene::Sphere2Plane(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data)
//{
//	Sphere* sphere = dynamic_cast<Sphere*>(object1);
//	Plane* plane = dynamic_cast<Plane*>(object2);
//	if (sphere && plane)
//	{
//		vec3 tempVec = plane->m_normal;
//		float temp = dot(sphere->GetPosition(), plane->m_normal) - plane->m_distanceToOrigin;
//		if (temp < 0)
//		{
//			tempVec *= -1.f;
//			temp *= -1.f;
//		}
//		if (sphere->GetRadius() > temp)
//		{
//			if (data)
//			{
//				data->collisionNorm = tempVec;
//				vec3 tempVec2 = plane->m_normal;
//				if (temp < 0)
//					tempVec2 *= -1.f;
//				data->forceVec = -1.f * sphere->GetMass() * tempVec2 * dot(tempVec2, sphere->GetVelocity());
//			}
//			return true;
//		}
//		//vec3 temp(sphere1->GetPosition() - vec3(0.f, plane2->m_distanceToOrigin, 0.f));
//		//return (dot(plane2->m_normal, temp) < sphere1->GetRadius());
//	}
//	return false;
//}

bool PhysicsScene::Sphere2Sphere(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data)
{
	Sphere* sphere1 = dynamic_cast<Sphere*>(object1);
	Sphere* sphere2 = dynamic_cast<Sphere*>(object2);
	if (sphere1 && sphere2)
	{
		float temp1 = glm::distance(sphere1->GetPosition(), sphere2->GetPosition());
		float temp2 = (sphere1->GetRadius() + sphere2->GetRadius());
		if (temp1 <= temp2)
		{
			if (data)
			{
				data->intersection = temp2 - temp1;
				data->bounciness = min(sphere1->GetBouciness(), sphere2->GetBouciness());
				data->collisionNorm = normalize(sphere2->GetPosition() - sphere1->GetPosition());
				data->relativeVelo = sphere1->GetVelocity() - sphere2->GetVelocity();
				data->velAlongNormal = dot(data->relativeVelo, data->collisionNorm);
				data->collisionVec = data->collisionNorm * data->velAlongNormal * data->bounciness;
				data->forceVec = data->collisionVec * 1.f / (sphere1->GetInverseMass() + sphere2->GetInverseMass());
			}
			return true;
		}
	}
	return false;
}

bool PhysicsScene::Sphere2Box(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data)
{
	Sphere* sphere = dynamic_cast<Sphere*>(object1);
	Box* box = dynamic_cast<Box*>(object2);
	if (sphere && box)
	{
		vec3 pc = glm::clamp(sphere->GetPosition(), box->GetMinVert(), box->GetMaxVert());
		//vec3 pc = box->ClosestPoint(sphere->GetPosition());
		float temp1 = glm::distance(pc, sphere->GetPosition());
		float temp2 = sphere->GetRadius();
		if (temp1 <= temp2)
		{
			if (data)
			{
				data->intersection = temp2 - temp1;
				data->bounciness = min(sphere->GetBouciness(), box->GetBouciness());
				data->collisionNorm = normalize(box->GetPosition() - sphere->GetPosition());
				data->relativeVelo = sphere->GetVelocity() - box->GetVelocity();
				data->velAlongNormal = dot(data->relativeVelo, data->collisionNorm);
				data->collisionVec = data->collisionNorm * data->velAlongNormal * data->bounciness;
				data->forceVec = data->collisionVec / (sphere->GetInverseMass() + box->GetInverseMass());
			}
			return true;
		}
	}
	return false;
}

//bool PhysicsScene::Box2Plane(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data)
//{
//	Box* box = dynamic_cast<Box*>(object1);
//	Plane* plane = dynamic_cast<Plane*>(object2);
//	if (box && plane)
//	{
//		float temp1 = dot(plane->m_normal, (box->GetPosition() - vec3(0.f, plane->m_distanceToOrigin, 0.f)));
//		float temp2 = ((box->m_length / 2.f) * glm::abs(dot(plane->m_normal, vec3(1.f, 0.f, 0.f))));
//		temp2 += ((box->m_height / 2.f) * glm::abs(dot(plane->m_normal, vec3(0.f, 1.f, 0.f))));
//		temp2 += ((box->m_width / 2.f) * glm::abs(dot(plane->m_normal, vec3(0.f, 0.f, 1.f))));
//		return (temp1 < temp2);
//	}
//	return false;
//}

bool PhysicsScene::Box2Sphere(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data)
{
	Box* box = dynamic_cast<Box*>(object1);
	Sphere* sphere = dynamic_cast<Sphere*>(object2);
	if (box && sphere)
	{
		vec3 pc = glm::clamp(sphere->GetPosition(), box->GetMinVert(), box->GetMaxVert());
		//vec3 pc = box->ClosestPoint(sphere->GetPosition());
		float temp1 = glm::distance(sphere->GetPosition(), pc);
		float temp2 = sphere->GetRadius();
		if (temp1 <= temp2)
		{
			if (data)
			{
				data->intersection = temp2 - temp1;
				data->bounciness = min(box->GetBouciness(), sphere->GetBouciness());
				data->collisionNorm = normalize(sphere->GetPosition() - box->GetPosition());
				data->relativeVelo = box->GetVelocity() - sphere->GetVelocity();
				data->velAlongNormal = dot(data->relativeVelo, data->collisionNorm);
				data->collisionVec = data->collisionNorm * data->velAlongNormal * data->bounciness;
				data->forceVec = data->collisionVec / (box->GetInverseMass() + sphere->GetInverseMass());
			}
			return true;
		}
	}
	return false;
}

bool PhysicsScene::Box2Box(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data)
{
	Box* box1 = dynamic_cast<Box*>(object1);
	Box* box2 = dynamic_cast<Box*>(object2);
	if (box1 && box2)
	{
		bool temp1 = (box1->GetMaxVert().x >= box2->GetMinVert().x && box2->GetMaxVert().x >= box1->GetMinVert().x);
		bool temp2 = (box1->GetMaxVert().y >= box2->GetMinVert().y && box2->GetMaxVert().y >= box1->GetMinVert().y);
		bool temp3 = (box1->GetMaxVert().z >= box2->GetMinVert().z && box2->GetMaxVert().z >= box1->GetMinVert().z);
		return (temp1 && temp2 && temp3);
	}
	return false;
}
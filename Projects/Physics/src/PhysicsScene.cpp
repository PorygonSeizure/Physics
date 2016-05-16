#include "PhysicsScene.h"
//#include "PhysicsObject.h"
#include "RigidBody.h"
//#include "Plane.h"
#include "Sphere.h"
#include "Box.h"

using namespace Physics;
using glm::vec3;
using glm::dot;
using glm::normalize;

void PhysicsScene::Simulate(vec3 gravity, float deltaTime)
{
	for (auto iter = m_physicsObjects.begin(); iter != m_physicsObjects.end(); iter++)
	{
		RigidBody* obj = (*iter);
		obj->Update(gravity, deltaTime);

		const vec3& pos = obj->GetPosition();
		const vec3& vel = obj->GetVelocity();
		if (pos.y < 0)
		{
			obj->SetPosition(vec3(pos.x, 0.f, pos.z));
			obj->SetVelocity(vec3(vel.x, -vel.y, vel.z));
		}
	}

	CheckForCollision();
	ResolveCollisions();
}

void PhysicsScene::DestroyPhysicsObject(RigidBody* object)
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

	m_collisions.clear();
	bool collision = false;
	
	//for (int outer = 0; outer < objectsCount - 1; outer++)
	for (auto iter0 = m_physicsObjects.begin(); iter0 != m_physicsObjects.end(); iter0++)
	{
		//for (int inner = outer + 1; inner < objectsCount; inner++)
		for (auto iter1 = iter0 + 1; iter1 != m_physicsObjects.end(); iter1++)
		{
			RigidBody* object1 = (*iter0);
			RigidBody* object2 = (*iter1);
			
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
						//if ((*iter1)->GetShapeID() == PLANE && (*iter2)->GetShapeID() == SPHERE)
						//{
						//	(*iter2)->ApplyForce(2.f * temp.forceVec);
						//	(*iter2)->SetPosition((*iter2)->GetPosition() + (temp.collisionNorm * temp.intersection * 0.5f));
						//}
						if ((*iter1)->GetShapeID() == SPHERE && (*iter2)->GetShapeID() == SPHERE)
						{
							(*iter1)->ApplyForceToActor((*iter2), 2.f * temp.forceVec);
							vec3 seperationVec = temp.collisionNorm * temp.intersection * 0.5f;

							(*iter1)->SetPosition((*iter1)->GetPosition() - seperationVec);
							(*iter2)->SetPosition((*iter2)->GetPosition() + seperationVec);
						}
						if (((*iter1)->GetShapeID() == SPHERE && (*iter2)->GetShapeID() == BOX) || ((*iter1)->GetShapeID() == BOX && (*iter2)->GetShapeID() == SPHERE))
						{
							(*iter1)->ApplyForceToActor((*iter2), 2.f * temp.forceVec);
							vec3 seperationVec = temp.collisionNorm * temp.intersection * 0.5f;

							(*iter1)->SetPosition((*iter1)->GetPosition() - seperationVec);
							(*iter2)->SetPosition((*iter2)->GetPosition() + seperationVec);
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
//	if (plane1 != NULL && plane2 != NULL)
//	{
//		return (plane1->m_normal != plane2->m_normal || plane1->m_normal != -plane2->m_normal);
//	}
//	return false;
//}

//bool PhysicsScene::Plane2Sphere(PhysicsObject* object1, PhysicsObject* object2, IntersectData* data)
//{
//	Plane* plane = dynamic_cast<Plane*>(object1);
//	Sphere* sphere = dynamic_cast<Sphere*>(object2);
//	if (plane != NULL && sphere != NULL)
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
//	if (plane != NULL && box != NULL)
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
//	if (sphere != NULL && plane != NULL)
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
//			if (data != nullptr)
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

bool PhysicsScene::Sphere2Sphere(RigidBody* object1, RigidBody* object2, IntersectData* data)
{
	Sphere* sphere1 = dynamic_cast<Sphere*>(object1);
	Sphere* sphere2 = dynamic_cast<Sphere*>(object2);
	if (sphere1 != NULL && sphere2 != NULL)
	{
		float temp1 = glm::distance(sphere1->GetPosition(), sphere2->GetPosition());
		float temp2 = (sphere1->GetRadius() + sphere2->GetRadius());
		if (temp1 < temp2)
		{
			if (data != nullptr)
			{
				data->intersection = temp2 - temp1;
				data->collisionNorm = normalize(sphere2->GetPosition() - sphere1->GetPosition());
				data->relativeVelo = sphere1->GetVelocity() - sphere2->GetVelocity();
				data->collisionVec = data->collisionNorm * dot(data->relativeVelo, data->collisionNorm);
				data->forceVec = data->collisionVec * 1.f / (sphere1->GetInverseMass() + sphere2->GetInverseMass());
			}
			return true;
		}
	}
	return false;
}

bool PhysicsScene::Sphere2Box(RigidBody* object1, RigidBody* object2, IntersectData* data)
{
	Sphere* sphere = dynamic_cast<Sphere*>(object1);
	Box* box = dynamic_cast<Box*>(object2);
	if (sphere != NULL && box != NULL)
	{
		vec3 pc(glm::clamp(sphere->GetPosition(), box->GetMinVert(), box->GetMaxVert()));
		vec3 temp0(sphere->GetPosition() - pc);
		float temp1 = dot(temp0, temp0);
		float temp2 = (sphere->GetRadius() * sphere->GetRadius());
		if (temp1 < temp2)
		{
			if (data != nullptr)
			{
				data->intersection = temp2 - temp1;
				data->collisionNorm = normalize(box->GetPosition() - sphere->GetPosition());
				data->relativeVelo = sphere->GetVelocity() - box->GetVelocity();
				data->collisionVec = data->collisionNorm * dot(data->relativeVelo, data->collisionNorm);
				data->forceVec = data->collisionVec * 1.f / (sphere->GetInverseMass() + box->GetInverseMass());
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
//	if (box != NULL && plane != NULL)
//	{
//		float temp1 = dot(plane->m_normal, (box->GetPosition() - vec3(0.f, plane->m_distanceToOrigin, 0.f)));
//		float temp2 = ((box->m_length / 2.f) * glm::abs(dot(plane->m_normal, vec3(1.f, 0.f, 0.f))));
//		temp2 += ((box->m_height / 2.f) * glm::abs(dot(plane->m_normal, vec3(0.f, 1.f, 0.f))));
//		temp2 += ((box->m_width / 2.f) * glm::abs(dot(plane->m_normal, vec3(0.f, 0.f, 1.f))));
//		return (temp1 < temp2);
//	}
//	return false;
//}

bool PhysicsScene::Box2Sphere(RigidBody* object1, RigidBody* object2, IntersectData* data)
{
	Box* box = dynamic_cast<Box*>(object1);
	Sphere* sphere = dynamic_cast<Sphere*>(object2);
	if (box != NULL && sphere != NULL)
	{
		vec3 pc(glm::clamp(sphere->GetPosition(), box->GetMinVert(), box->GetMaxVert()));
		vec3 temp0(sphere->GetPosition() - pc);
		float temp1 = dot(temp0, temp0);
		float temp2 = (sphere->GetRadius() * sphere->GetRadius());
		if (temp1 < temp2)
		{
			if (data != nullptr)
			{
				data->intersection = temp2 - temp1;
				data->collisionNorm = normalize(sphere->GetPosition() - box->GetPosition());
				data->relativeVelo = box->GetVelocity() - sphere->GetVelocity();
				data->collisionVec = data->collisionNorm * dot(data->relativeVelo, data->collisionNorm);
				data->forceVec = data->collisionVec * 1.f / (box->GetInverseMass() + sphere->GetInverseMass());
			}
			return true;
		}
	}
	return false;
}

bool PhysicsScene::Box2Box(RigidBody* object1, RigidBody* object2, IntersectData* data)
{
	Box* box1 = dynamic_cast<Box*>(object1);
	Box* box2 = dynamic_cast<Box*>(object2);
	if (box1 != NULL && box2 != NULL)
	{
		bool temp1 = (box1->GetMaxVert().x > box2->GetMinVert().x && box2->GetMaxVert().x > box1->GetMinVert().x);
		bool temp2 = (box1->GetMaxVert().y > box2->GetMinVert().y && box2->GetMaxVert().y > box1->GetMinVert().y);
		bool temp3 = (box1->GetMaxVert().z > box2->GetMinVert().z && box2->GetMaxVert().z > box1->GetMinVert().z);
		return (temp1 && temp2 && temp3);
	}
	return false;
}
#include "Collision.h"
#include <iostream>
#include <vector>

using physx::PxU32;
using physx::PxPairFlag;
using physx::PxFilterData;
using physx::PxShape;

void Collision::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, PxU32 numberPairs)
{
	for (PxU32 i = 0; i < numberPairs; i++)
	{
		const physx::PxContactPair& cp = pairs[i];
		//only interested in touches found and lost
		if (cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
			std::cout << "Collision Detected between: " << pairHeader.actors[0]->getName() << pairHeader.actors[1]->getName() << std::endl;
	}
}

void Collision::onTrigger(physx::PxTriggerPair* pairs, PxU32 numberPairs)
{
	for (PxU32 i = 0; i < numberPairs; i++)
	{
		physx::PxTriggerPair* pair = &pairs[i];
		physx::PxActor* triggerActor = pair->triggerActor;
		physx::PxActor* otherActor = pair->otherActor;

		if (otherActor->getName() && triggerActor->getName())
			std::cout << otherActor->getName() << " Entered Trigger " << triggerActor->getName() << std::endl;
	}
}

physx::PxFilterFlags FilterShader(physx::PxFilterObjectAttributes attributes0, PxFilterData filterData0, physx::PxFilterObjectAttributes attributes1, PxFilterData filterData1, 
									physx::PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	//let triggers through
	if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		return physx::PxFilterFlag::eDEFAULT;
	}
	//generate contacts for all that were not filtered above
	pairFlags = PxPairFlag::eCONTACT_DEFAULT;
	//trigger the contact callback for pairs (A,B) where
	//the filtermask of A contains the ID of B and vice versa.
	if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_LOST;
	return physx::PxFilterFlag::eDEFAULT;
}

void SetupFiltering(physx::PxRigidActor* actor, PxU32 filterGroup, PxU32 filterMask)
{
	PxFilterData filterData;
	filterData.word0 = filterGroup;	//word0 = own ID
	filterData.word1 = filterMask;	//word1 = ID mask to filter pairs that trigger a contact callback
	const PxU32 numberShapes = actor->getNbShapes();
	std::vector<PxShape*> shapes(numberShapes);
	actor->getShapes(&shapes[0], numberShapes);
	for (PxU32 i = 0; i < numberShapes; i++)
	{
		PxShape* shape = shapes[i];
		shape->setSimulationFilterData(filterData);
	}
}

void SetShapeAsTrigger(physx::PxRigidActor* actorIn)
{
	physx::PxRigidStatic* staticActor = actorIn->is<physx::PxRigidStatic>();
	//assert(staticActor);
	if (staticActor)
	{
		const PxU32 numberShapes = staticActor->getNbShapes();
		std::vector<PxShape*> shapes(numberShapes);
		staticActor->getShapes(&shapes[0], numberShapes);
		for (PxU32 i = 0; i < numberShapes; i++)
		{
			shapes[i]->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
			shapes[i]->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
		}
	}
}

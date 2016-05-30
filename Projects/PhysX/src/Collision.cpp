#include "Collision.h"
#include <iostream>

using namespace physx;

void Collision::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 numberPairs)
{
	for (PxU32 i = 0; i < numberPairs; i++)
	{
		const PxContactPair& cp = pairs[i];
		//only interested in touches found and lost
		if (cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
			std::cout << "Collision Detected between: " << pairHeader.actors[0]->getName() << pairHeader.actors[1]->getName() << std::endl;
	}
}

void Collision::onTrigger(PxTriggerPair* pairs, PxU32 numberPairs)
{
	for (PxU32 i = 0; i < numberPairs; i++)
	{
		PxTriggerPair* pair = pairs + i;
		PxActor* triggerActor = pair->triggerActor;
		PxActor* otherActor = pair->otherActor;
		std::cout << otherActor->getName() << " Entered Trigger " << triggerActor->getName() << std::endl;
	}
}
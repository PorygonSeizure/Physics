#ifndef _COLLISION_H_
#define _COLLISION_H_

#include <PxPhysicsAPI.h>

class Collision : public physx::PxSimulationEventCallback
{
public:
	Collision() {}
	virtual ~Collision() {}
	
	virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 numberPairs);
	virtual void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 numberPairs);
	virtual void onConstraintBreak(physx::PxConstraintInfo*, physx::PxU32) {}
	virtual void onWake(physx::PxActor**, physx::PxU32) {}
	virtual void onSleep(physx::PxActor**, physx::PxU32) {}
};

#endif
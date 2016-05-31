#ifndef _COLLISION_H_
#define _COLLISION_H_

#include <PxPhysicsAPI.h>

struct FilterGroup
{
	enum Enum
	{
		ePLAYER = (1 << 0),
		ePLATFORM = (1 << 1),
		eGROUND = (1 << 2),
		eBLOCK = (1 << 3)
	};
};

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

physx::PxFilterFlags FilterShader(physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0, physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, 
									physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize);
void SetupFiltering(physx::PxRigidActor* actor, physx::PxU32 filterGroup, physx::PxU32 filterMask);
void SetShapeAsTrigger(physx::PxRigidActor* actorIn);

#endif
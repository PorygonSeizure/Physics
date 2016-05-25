#ifndef _RAGDOLL_H_
#define _RAGDOLL_H_

#include <PxPhysicsAPI.h>

//Parts which make up our ragdoll
enum RagdollParts
{
	NO_PARENT = -1,
	LOWER_SPINE,
	LEFT_PELVIS,
	RIGHT_PELVIS,
	LEFT_UPPER_LEG,
	RIGHT_UPPER_LEG,
	LEFT_LOWER_LEG,
	RIGHT_LOWER_LEG,
	UPPER_SPINE,
	LEFT_CLAVICLE,
	RIGHT_CLAVICLE,
	NECK,
	HEAD,
	LEFT_UPPER_ARM,
	RIGHT_UPPER_ARM,
	LEFT_LOWER_ARM,
	RIGHT_LOWER_ARM
};

struct RagdollNode
{
	physx::PxQuat globalRotation;	//rotation of this link in model space - we could have done this relative to the parent node but it's harder to visualize when setting up the data by hand

	physx::PxVec3 scaledGlobalPos;	//Position of the link centre in world space which is calculated when we process the node.It's easiest if we store it here so we have it when we transform the 
									//child

	int parentNodeIDX;	//Index of the parent node

	float halfLength;	//half length of the capsule for this node
	float radius;	//radius of capsule for this node
	float parentLinkPos;	//relative position of link centre in parent to this node. 0 is the centre of hte node, -1 is left end of capsule and 1 is right end of capsule relative to x
	float childLinkPos;	//relative position of link centre in child

	char* name;	 //name of link

	physx::PxArticulationLink* linkPtr;

	//constructor
	RagdollNode(physx::PxQuat globalRotationInput, int parentNodeIDXInput, float halfLengthInput, float radiusInput, float parentLinkPosInput, float childLinkPosInput, char* nameInput)
	{
		globalRotation = globalRotationInput;
		parentNodeIDX = parentNodeIDXInput;
		halfLength = halfLengthInput;
		radius = radiusInput;
		parentLinkPos = parentLinkPosInput;
		childLinkPos = childLinkPosInput;
		name = nameInput;
	}
};

physx::PxArticulation* MakeRagdoll(physx::PxPhysics*, RagdollNode**, physx::PxTransform, float, physx::PxMaterial*);

class Ragdoll
{
public:
	Ragdoll();
	~Ragdoll();
};

#endif
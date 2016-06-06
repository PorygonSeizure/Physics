#include "Ragdoll.h"
#include "Collision.h"

using physx::PxTransform;
using physx::PxVec3;

physx::PxArticulation* MakeRagdoll(physx::PxPhysics* physics, std::vector<RagdollNode*> nodeVector, PxTransform worldPos, float scaleFactor, physx::PxMaterial* ragdollMaterial)
{
	//create the articulation for our ragdoll
	physx::PxArticulation* articulation = physics->createArticulation();
	std::vector<RagdollNode*> copyVector = nodeVector;

	//while there are more nodes to process...
	for (unsigned int i = 0; i < copyVector.size(); i++)
	{
		//get a pointer to the current node
		RagdollNode* currentNodePtr = copyVector[i];

		//create a pointer ready to hold the parent node pointer if there is one
		RagdollNode* parentNode = nullptr;

		//get scaled values for capsule
		float radius = currentNodePtr->radius * scaleFactor;
		float halfLength = currentNodePtr->halfLength * scaleFactor;
		float childHalfLength = radius + halfLength;
		float parentHalfLength = 0;	//will be set later if there is a parent

		//get a pointer to the parent
		physx::PxArticulationLink* parentLinkPtr = NULL;
		currentNodePtr->scaledGlobalPos = worldPos.p;

		if (currentNodePtr->parentNodeIDX != -1)
		{
			//if there is a parent then we need to work out our local position for the link
			//get a pointer to the parent node
			parentNode = nodeVector[currentNodePtr->parentNodeIDX];

			//get a pointer to the link for the parent
			parentLinkPtr = parentNode->linkPtr;
			parentHalfLength = (parentNode->radius + parentNode->halfLength) * scaleFactor;

			//work out the local position of the node
			PxVec3 currentRelative = currentNodePtr->childLinkPos * currentNodePtr->globalRotation.rotate(PxVec3(childHalfLength, 0, 0));
			PxVec3 parentRelative = -currentNodePtr->parentLinkPos * parentNode->globalRotation.rotate(PxVec3(parentHalfLength, 0, 0));
			currentNodePtr->scaledGlobalPos = parentNode->scaledGlobalPos - (parentRelative + currentRelative);
		}

		//build the transform for the link
		PxTransform linkTransform = PxTransform(currentNodePtr->scaledGlobalPos, currentNodePtr->globalRotation);

		//create the link in the articulation
		physx::PxArticulationLink* link = articulation->createLink(parentLinkPtr, linkTransform);

		//add the pointer to this link into the ragdoll data so we have it for later when we want to link to it
		currentNodePtr->linkPtr = link;
		float jointSpace = 0.01f;	//gap between joints
		float capsuleHalfLength = (halfLength > jointSpace ? halfLength - jointSpace : 0) + 0.01f;
		physx::PxCapsuleGeometry capsule(radius, capsuleHalfLength);
		link->createShape(capsule, *ragdollMaterial);	//adds a capsule collider to the link
		link->setName(currentNodePtr->name);
		SetupFiltering(link, FilterGroup::ePLAYER, FilterGroup::eGROUND);
		SetupFiltering(link, FilterGroup::ePLAYER, FilterGroup::eGROUND | FilterGroup::ePLATFORM);
		physx::PxRigidBodyExt::updateMassAndInertia(*link, 50.f);	//adds some mass, mass should really be part of the data!

		if (currentNodePtr->parentNodeIDX != -1)
		{
			//get the pointer to the joint from the link
			physx::PxArticulationJoint* joint = link->getInboundJoint();

			//get the relative rotation of this link
			physx::PxQuat frameRotation = parentNode->globalRotation.getConjugate() * currentNodePtr->globalRotation;

			//set the parent contraint frame
			PxTransform parentConstraintFrame = PxTransform(PxVec3(currentNodePtr->parentLinkPos * parentHalfLength, 0, 0), frameRotation);

			//set the child constraint frame (this the constraint frame of the newly added link)
			PxTransform thisConstraintFrame = PxTransform(PxVec3(currentNodePtr->childLinkPos * childHalfLength, 0, 0));

			//set up the poses for the joint so it is in the correct place
			joint->setParentPose(parentConstraintFrame);
			joint->setChildPose(thisConstraintFrame);

			//set up some constraints to stop it flopping around
			joint->setStiffness(20);
			joint->setDamping(20);
			joint->setSwingLimit(0.4f, 0.4f);
			joint->setSwingLimitEnabled(true);
			joint->setTwistLimit(-0.1f, 0.1f);
			joint->setTwistLimitEnabled(true);
		}
	}
	return articulation;
}
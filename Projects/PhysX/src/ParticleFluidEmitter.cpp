#include "ParticleFluidEmitter.h"
#include <iostream>
#include <vector>
#include <Gizmos.h>

using physx::PxVec3;
using physx::PxU32;
using physx::PxStrideIterator;

//constructor
ParticleFluidEmitter::ParticleFluidEmitter(int maxParticles, PxVec3 position, physx::PxParticleFluid* pf, float releaseDelay)
{
	m_releaseDelay = releaseDelay;
	m_maxParticles = maxParticles;		//maximum number of particles our emitter can handle
	//allocate an array
	m_activeParticles = new Particle[m_maxParticles];	//array of particle structs
	m_time = 0;	//time system has been running
	m_respawnTime = 0;	//time for next respawn
	m_position = position;
	m_pf = pf;	//pointer to the physX particle system
	m_particleMaxAge = 8;	//maximum time in seconds that a particle can live for
	//initialize the buffer
	for (int index = 0; index < m_maxParticles; index++)
		m_activeParticles[index].active = false;
}

//add particle to PhysX System
bool ParticleFluidEmitter::AddPhysXParticle(int particleIndex)
{
	//reserve space for data
	physx::PxParticleCreationData particleCreationData;
	//set up the data
	particleCreationData.numParticles = 1;	//spawn one particle at a time,  this is inefficient and we could improve this by passing in the list of particles.
	//set up the buffers
	PxU32 myIndexBuffer[] = { (PxU32)particleIndex };
	PxVec3 startPos = m_position;
	PxVec3 startVel(0, 0, 0);
	//randomize starting velocity.
	float fT = (rand() % (RAND_MAX + 1)) / (float)RAND_MAX;
	startVel.x += m_minVelocity.x + (fT * (m_maxVelocity.x - m_minVelocity.x));
	fT = (rand() % (RAND_MAX + 1)) / (float)RAND_MAX;
	startVel.y += m_minVelocity.y + (fT * (m_maxVelocity.y - m_minVelocity.y));
	fT = (rand() % (RAND_MAX + 1)) / (float)RAND_MAX;
	startVel.z += m_minVelocity.z + (fT * (m_maxVelocity.z - m_minVelocity.z));

	//we can change starting position tos get different emitter shapes
	PxVec3 myPositionBuffer[] = { startPos };
	PxVec3 myVelocityBuffer[] = { startVel };

	particleCreationData.indexBuffer = PxStrideIterator<const PxU32>(myIndexBuffer);
	particleCreationData.positionBuffer = PxStrideIterator<const PxVec3>(myPositionBuffer);
	particleCreationData.velocityBuffer = PxStrideIterator<const PxVec3>(myVelocityBuffer);
	// create particles in *PxParticleSystem* ps
	return m_pf->createParticles(particleCreationData);
}

//updateParticle
void ParticleFluidEmitter::Update(float delta)
{
	//tick the emitter
	m_time += delta;
	m_respawnTime += delta;
	int numberSpawn = 0;
	//if respawn time is greater than our release delay then we spawn at least one particle so work out how many to spawn
	if (m_respawnTime > m_releaseDelay)
	{
		numberSpawn = (int)(m_respawnTime / m_releaseDelay);
		m_respawnTime -= (numberSpawn * m_releaseDelay);
	}
	// spawn the required number of particles 
	for (int count = 0; count < numberSpawn; count++)
	{
		//get the next free particle
		int particleIndex = GetNextFreeParticle();
		if (particleIndex >= 0) { AddPhysXParticle(particleIndex); }	//if we got a particle ID then spawn it
	}
	//check to see if we need to release particles because they are either too old or have hit the particle sink
	//lock the particle buffer so we can work on it and get a pointer to read data
	physx::PxParticleReadData* rd = m_pf->lockParticleReadData();
	// access particle data from PxParticleReadData was OK
	if (rd)
	{
		std::vector<PxU32> particlesToRemove;	//we need to build a list of particles to remove so we can do it all in one go
		PxStrideIterator<const physx::PxParticleFlags> flagsIt(rd->flagsBuffer);
		PxStrideIterator<const PxVec3> positionIt(rd->positionBuffer);

		for (unsigned i = 0; i < rd->validParticleRange; ++i, ++flagsIt, ++positionIt)
		{
			if (*flagsIt & physx::PxParticleFlag::eVALID)
			{
				//if particle is either too old or has hit the sink then mark it for removal.  We can't remove it here because we buffer is locked
				if (*flagsIt & physx::PxParticleFlag::eCOLLISION_WITH_DRAIN)
				{
					//mark our local copy of the particle free
					ReleaseParticle(i);
					//add to our list of particles to remove
					particlesToRemove.push_back(i);
				}
			}
		}
		// return ownership of the buffers back to the SDK
		rd->unlock();
		//if we have particles to release then pass the particles to remove to PhysX so it can release them
		if (particlesToRemove.size() > 0)
		{
			//create a buffer of particle indicies which we are going to remove
			PxStrideIterator<const PxU32> indexBuffer(&particlesToRemove[0]);
			//free particles from the physics system
			m_pf->releaseParticles(particlesToRemove.size(), indexBuffer);
		}
	}
}

//simple routine to render our particles
void ParticleFluidEmitter::RenderParticles()
{
	//lock SDK buffers of *PxParticleSystem* ps for reading
	physx::PxParticleFluidReadData* fd = m_pf->lockParticleFluidReadData();
	//access particle data from PxParticleReadData
	float minX = 1000;
	float maxX = -1000;
	float minZ = 1000;
	float maxZ = -1000;
	float minY = 1000;
	float maxY = -1000;
	if (fd)
	{
		PxStrideIterator<const physx::PxParticleFlags> flagsIt(fd->flagsBuffer);
		PxStrideIterator<const PxVec3> positionIt(fd->positionBuffer);
		PxStrideIterator<const physx::PxF32> densityIt(fd->densityBuffer);
		for (unsigned i = 0; i < fd->validParticleRange; ++i, ++flagsIt, ++positionIt, ++densityIt)
		{
			if (*flagsIt & physx::PxParticleFlag::eVALID)
			{
				//density tells us how many neighbours a particle has.  
				//If it has a density of 0 it has no neighbours, 1 is maximum neighbouts
				//we can use this to decide if the particle is seperate or part of a larger body of fluid
				glm::vec3 pos(positionIt->x, positionIt->y, positionIt->z);
				Gizmos::AddAABBFilled(pos, glm::vec3(.12, .12, .12), glm::vec4(1, 0, 1, 1));
			}
		}
		//return ownership of the buffers back to the SDK
		fd->unlock();
	}
}
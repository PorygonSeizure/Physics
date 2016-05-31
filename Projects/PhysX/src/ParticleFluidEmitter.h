#ifndef _PARTICLE_FLUID_EMITTER_H_
#define _PARTICLE_FLUID_EMITTER_H_

#include "ParticleEmitter.h"
#include <glm/ext.hpp>

//simple class for particle emitter.  For a real system we would make this a base class and derive different emitters from it by making functions virtual and overloading them.
class ParticleFluidEmitter : public ParticleEmitter
{
	
public:
	ParticleFluidEmitter(int maxParticles, physx::PxVec3 position, physx::PxParticleFluid* pf,float releaseDelay);
	~ParticleFluidEmitter() {}

	virtual void Update(float delta);
	virtual void RenderParticles();

private:
	virtual bool AddPhysXParticle(int particleIndex);

	physx::PxParticleFluid* m_pf;

	int m_rows;
	int m_cols;
	int m_depth;
	int m_boxWidth;
	int m_boxHeight;
};

#endif
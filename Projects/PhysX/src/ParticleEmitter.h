#ifndef _PARTICLE_EMITTER_H_
#define _PARTICLE_EMITTER_H_

#include <PxPhysicsAPI.h>

//simple struct for our particles

struct Particle
{
	bool active;

	float maxTime;
};


//simple class for particle emitter. For a real system we would make this a base class and derive different emitters from it by making functions virtual and overloading them.
class ParticleEmitter
{
public:
	ParticleEmitter(int maxParticles, physx::PxVec3 position, physx::PxParticleSystem* ps, float releaseDelay);
	ParticleEmitter() {}
	virtual ~ParticleEmitter();
	
	virtual void SetStartVelocityRange(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
	virtual void Update(float delta);
	virtual void ReleaseParticle(int);
	virtual void RenderParticles();

	bool TooOld(int);

protected:
	virtual int GetNextFreeParticle();

	virtual bool AddPhysXParticle(int particleIndex);

	int m_maxParticles;
	int m_numberActiveParticles;

	Particle* m_activeParticles;

	float m_releaseDelay;
	float m_time;
	float m_respawnTime;
	float m_particleMaxAge;

	physx::PxVec3 m_position;
	physx::PxVec3 m_minVelocity;
	physx::PxVec3 m_maxVelocity;
	
	physx::PxParticleSystem* m_ps;
};

#endif
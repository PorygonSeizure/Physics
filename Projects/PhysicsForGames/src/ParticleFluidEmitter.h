#pragma once
#include <PxPhysicsAPI.h>
#include "glm/ext.hpp"
//using namespace physx;
using namespace std;

//simple struct for our particles

struct FluidParticle
{
	bool active;
	float maxTime;
};


//simple class for particle emitter.  For a real system we would make this a base class and derive different emitters from it by making functions virtual and overloading them.
class ParticleFluidEmitter
{
public:
	ParticleFluidEmitter(int maxParticles, physx::PxVec3 position, physx::PxParticleFluid* pf, float releaseDelay);
	~ParticleFluidEmitter();

	void Update(float delta);
	void ReleaseParticle(int);
	void RenderParticles();
	void SetStartVelocityRange(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);

	bool TooOld(int);

private:
	int GetNextFreeParticle();
	bool AddPhysXParticle(int particleIndex);

	physx::PxParticleFluid* m_pf;

	int m_rows;
	int m_cols;
	int m_depth;
	int m_maxParticles;
	int m_numberActiveParticles;
	int m_boxWidth;
	int m_boxHeight;

	FluidParticle* m_activeParticles;

	float m_releaseDelay;
	float m_time;
	float m_respawnTime;
	float m_particleMaxAge;

	physx::PxVec3 m_position;
	physx::PxVec3 m_minVelocity;
	physx::PxVec3 m_maxVelocity;
};
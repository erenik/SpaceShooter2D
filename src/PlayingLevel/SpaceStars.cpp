/**
	Breaking out particle effects initiation code for clarity.
	Emil Hedemalm
	2020-04-28
*/

#include "SpaceStars.h"

#include "StateManager.h"
#include "Graphics/GraphicsManager.h"
#include "Graphics/Messages/GMParticles.h"
#include "Graphics/Messages/GraphicsMessages.h"
#include "Graphics/Particles/Stars.h"

ParticleSystem* stars = nullptr;
ParticleEmitter* starEmitter = nullptr;

void ClearStars() {
	/// Clear old stars?
	if (stars == nullptr)
		return;
	QueueGraphics(new GMClearParticles(stars));
	Graphics.QueueMessage(new GMUnregisterParticleSystem(stars, true));
	stars = nullptr;

}
void NewStars(Vector3f starDir, float emissionSpeed, float starScale, Color color) {

	Stars * newStars = new Stars(true);
	stars = newStars;
	stars->deleteEmittersOnDeletion = true;
	Graphics.QueueMessage(new GMRegisterParticleSystem(stars, true));

	/// Add emitter
	StarEmitter * emitter = new StarEmitter(Vector3f());
	starEmitter = emitter;

	Graphics.QueueMessage(new GMAttachParticleEmitter(starEmitter, stars));


	ParticleEmitter* startEmitter = new ParticleEmitter();
	startEmitter->newType = true;
	startEmitter->instantaneous = true;
	startEmitter->constantEmission = 1400;
	startEmitter->positionEmitter.type = EmitterType::PLANE_XY;
	startEmitter->positionEmitter.SetScale(100.f);
	startEmitter->velocityEmitter.type = EmitterType::VECTOR;
	startEmitter->velocityEmitter.vec = starDir;
	startEmitter->SetEmissionVelocity(emissionSpeed);
	startEmitter->SetParticleLifeTime(60.f);
	startEmitter->SetScale(starScale);
	startEmitter->SetColor(color);
	QueueGraphics(new GMAttachParticleEmitter(startEmitter, stars));

	/// Update base emitter emitting all the time.
	starEmitter->newType = true;
	starEmitter->direction = starDir;
	starEmitter->SetEmissionVelocity(emissionSpeed);
	starEmitter->SetParticlesPerSecond(40);
	starEmitter->positionEmitter.type = EmitterType::PLANE_XY;
	starEmitter->positionEmitter.SetScale(30.f);
	starEmitter->velocityEmitter.type = EmitterType::VECTOR;
	starEmitter->velocityEmitter.vec = starDir;
	starEmitter->SetParticleLifeTime(60.f);
	starEmitter->SetColor(color);
	starEmitter->SetScale(starScale);

}


void LetStarsTrack(Entity* entity, Vector3f positionOffset) {
	GraphicsMan.QueueMessage(new GMSetParticleEmitter(starEmitter, GT_EMITTER_ENTITY_TO_TRACK, entity));
	GraphicsMan.QueueMessage(new GMSetParticleEmitter(starEmitter, GT_EMITTER_POSITION_OFFSET, Vector3f(70.f, 0, 0)));

	// Track ... level with effects.
	starEmitter->entityToTrack = entity;
	starEmitter->positionOffset = positionOffset;

}
#include "GameplaySystem.h"
#include "Components.h"

XMFLOAT3 faceDirection[] = {
	{0.f, 0.f, 1.f},							// front
	{-1.f, 0.f, 0.f},							// right
	{0.f, 1.f, 0.f},							// top
	{1.f, 0.f, 0.f},							// left
	{0.f, 0.f, -1.f},							// back
	{0.f, -1.f, 0.f}							// bottom
};

enum FaceDirection {
	FRONT,
	RIGHT,
	TOP,
	LEFT,
	BACK,
	BOTTOM,
	TOTAL
};

void GameplaySystem::onInit(std::shared_ptr<Registry> registry) {
	this->registry = registry;

	storeEntities();
	onReset();
}

void GameplaySystem::onReset() {
	// reset the entire cube by repainting the faces
	for (auto& e : entitiesFace) {
		auto& ctrans = registry->getComponent<CTransform>(e);
		auto& cdraw = registry->getComponent<CDraw>(e);

		for (uint8_t i = 0; i < _countof(faceDirection); i++) {
			if ((ctrans.forward.x == faceDirection[i].x) && (ctrans.forward.y == faceDirection[i].y) && (ctrans.forward.z == faceDirection[i].z)) {
				cdraw.colorIndex = i;
				break;
			}
		}
			
	}
}

void GameplaySystem::storeEntities() {
	// get all CFace entities
	// create signature for filtering entities with CDraw comp
	Signature signature;
	signature.set(registry->getComponentTypeID<CFace>());

	// store all drawable entities that have CDraw
	entitiesFace = registry->getEntitiesFromSignature(signature);



}
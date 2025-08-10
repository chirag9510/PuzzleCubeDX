#include "GameplaySystem.h"
#include "Components.h"
#include "Helper.h"

#include <cstdlib>
#include <ctime>

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

GameplaySystem::GameplaySystem() : cubeRotInMotion(false), rotTargetReached(false) {

}

void GameplaySystem::onShuffle() {
	std::srand(std::time(0));
	for (int i = 0; i < 24; i++) 
		queueCmd.push(cubeNotations[std::rand() % cubeNotations.size()]);

}

void GameplaySystem::processInputCmd(const std::string strcmd) {
	// rotate cube sides by filling up the queue with algo commands even when using the button interface
	if (queueCmd.empty()) {
		int i = 0;
		while (true) {
			if (i >= strcmd.length())
				break;
			else if(i == strcmd.length() - 1)
				queueCmd.push(strcmd.substr(i++, 1));
			else {
				if (strcmd[i + 1] == 'i') {
					queueCmd.push(strcmd.substr(i, 2));
					i += 2;
				}
				else {
					queueCmd.push(strcmd.substr(i++, 1));
				}
			}
		}
	}
	
}

bool GameplaySystem::onUpdate(const float& deltaTime) {
	if (queueCmd.empty() == false && cubeRotInMotion == false) {
		currRotation = { 0.f, 0.f, 0.f };
		rotateCubeSide(queueCmd.front());
		queueCmd.pop();
	}

	// rotate the pivot and all its child pieces will rotate too

	// rotTargetReached allows to update transformations in RenderSystem final time one more frame after we create pivot model mat
	if (rotTargetReached && cubeRotInMotion) {
		rotTargetReached = false;
		cubeRotInMotion = false;

		for (auto& epiece : entitiesPiecesInRot) {
			auto& cTrans = registry->getComponent<CTransform>(epiece);
			auto& cHrchy = registry->getComponent<CHierarchy>(epiece);

			// precision problems in model matrix with translation cause the cube to render in buggy state
			// all world position xyz values must be 0 or 0.5 or -0.5f, thats why something like 0.499999 must be rounded off to 0.50
			// otherwise rotateCubeSide() cannot identify which pieces to rotate if there position isnt precise
			cTrans.mxmodel._41 = roundoff(cTrans.mxmodel._41, 0.5f);
			cTrans.mxmodel._42 = roundoff(cTrans.mxmodel._42, 0.5f);
			cTrans.mxmodel._43 = roundoff(cTrans.mxmodel._43, 0.5f);
			cTrans.mxlocal = cTrans.mxmodel;																// finalize local matrix position after rot is complete to set its position in stone			
			cHrchy = -1;																					// remove the pivot parent so it doesnt multiply with its matrix
		}
	}

	if (cubeRotInMotion) {
		auto& cTrans = registry->getComponent<CTransform>(entityCntlPivot);
		if (rotVelocity.x != 0.f) 
			currRotation.x += rotVelocity.x * deltaTime;
		else if (rotVelocity.y != 0.f)
			currRotation.y += rotVelocity.y * deltaTime;
		else if (rotVelocity.z != 0.f) 
			currRotation.z += rotVelocity.z * deltaTime;

		if ((rotationTarget.x > 0.f && currRotation.x > rotationTarget.x) || (rotationTarget.x < 0.f && currRotation.x < rotationTarget.x) ||
			(rotationTarget.y > 0.f && currRotation.y > rotationTarget.y) || (rotationTarget.y < 0.f && currRotation.y < rotationTarget.y) ||
			(rotationTarget.z > 0.f && currRotation.z > rotationTarget.z) || (rotationTarget.z < 0.f && currRotation.z < rotationTarget.z))
		{
			currRotation = rotationTarget;
			rotTargetReached = true;
			rotationTarget = { 0.f, 0.f, 0.f };
		}
			

		// model matrix = scale * rot * trans
		auto mxmodel = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&currRotation)) /** XMMatrixTranslationFromVector(XMLoadFloat3(&cTrans.pos))*/;
		XMStoreFloat4x4(&cTrans.mxmodel, mxmodel);
	}

	return cubeRotInMotion;
}




void GameplaySystem::onInit(std::shared_ptr<Registry> registry, int mWndWidth, int mWndHeight) {
	this->registry = registry;
	this->mWndHeight = mWndHeight;
	this->mWndWidth = mWndWidth;

	createCubeNotations();
	storeEntities();
	onReset();
}


void GameplaySystem::onReset() {
	if (queueCmd.empty()) {
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
}


void GameplaySystem::storeEntities() {
	// get all CFace entities
	Signature sigFace;
	sigFace.set(registry->getComponentTypeID<CFace>());
	// store all drawable entities that have CFace
	entitiesFace = registry->getEntitiesFromSignature(sigFace);


	// pieces 
	Signature sigPiece;
	sigPiece.set(registry->getComponentTypeID<CPiece>());
	// store all drawable entities that have CDraw
	entitiesPiece = registry->getEntitiesFromSignature(sigPiece);


	// central pivot entity to rotate all pieces of the cube
	Signature sigCenPivot;
	sigCenPivot.set(registry->getComponentTypeID<CCentralPivot>());
	auto vecEntCenPivot = registry->getEntitiesFromSignature(sigCenPivot);

	// there is only 1 central pivot entity
	entityCntlPivot = vecEntCenPivot[0];
}

void GameplaySystem::createCubeNotations() {
	// clockwise 
	cubeNotations.push_back("R");
	cubeNotations.push_back("L");
	cubeNotations.push_back("U");
	cubeNotations.push_back("F");
	cubeNotations.push_back("D");
	cubeNotations.push_back("B");

	// anti clockwise
	cubeNotations.push_back("Ri");
	cubeNotations.push_back("Li");
	cubeNotations.push_back("Ui");
	cubeNotations.push_back("Fi");
	cubeNotations.push_back("Di");
	cubeNotations.push_back("Bi");

	// slice
	cubeNotations.push_back("M");
	cubeNotations.push_back("Mi");
	cubeNotations.push_back("E");
	cubeNotations.push_back("Ei");
	cubeNotations.push_back("S");
	cubeNotations.push_back("Si");

	// double layer
	cubeNotations.push_back("r");
	cubeNotations.push_back("l");
	cubeNotations.push_back("u");
	cubeNotations.push_back("f");
	cubeNotations.push_back("d");
	cubeNotations.push_back("b");
	
	// inverse double layer
	cubeNotations.push_back("ri");
	cubeNotations.push_back("li");
	cubeNotations.push_back("ui");
	cubeNotations.push_back("fi");
	cubeNotations.push_back("di");
	cubeNotations.push_back("bi");

	// whole cube
	cubeNotations.push_back("X");
	cubeNotations.push_back("Xi");
	cubeNotations.push_back("Y");
	cubeNotations.push_back("Yi");
	cubeNotations.push_back("Z");
	cubeNotations.push_back("Zi");
}


void GameplaySystem::rotateCubeSide(std::string notation) {
	CTransform& ctrans = registry->getComponent<CTransform>(entityCntlPivot);

	if (notation == "R") {
		setPieceEntityPivot('x', -0.5f);
		rotationTarget.x = -XM_PIDIV2;
	}
	else if (notation == "L") {
		setPieceEntityPivot('x', 0.5f);
		rotationTarget.x = XM_PIDIV2;
	}
	else if (notation == "U") {
		setPieceEntityPivot('y', .5f);
		rotationTarget.y = XM_PIDIV2;
	}
	else if (notation == "F") {
		setPieceEntityPivot('z', .5f);
		rotationTarget.z = XM_PIDIV2;
	}
	else if (notation == "D") {
		setPieceEntityPivot('y', -.5f);
		rotationTarget.y = -XM_PIDIV2;
	}
	else if (notation == "B") {
		setPieceEntityPivot('z', -.5f);
		rotationTarget.z = -XM_PIDIV2;
	}

	if (notation == "Ri") {
		setPieceEntityPivot('x', -0.5f);
		rotationTarget.x = XM_PIDIV2;
	}
	else if (notation == "Li") {
		setPieceEntityPivot('x', 0.5f);
		rotationTarget.x = -XM_PIDIV2;
	}
	else if (notation == "Ui") {
		setPieceEntityPivot('y', .5f);
		rotationTarget.y = -XM_PIDIV2;
	}
	else if (notation == "Fi") {
		setPieceEntityPivot('z', .5f);
		rotationTarget.z = -XM_PIDIV2;
	}
	else if (notation == "Di") {
		setPieceEntityPivot('y', -.5f);
		rotationTarget.y = XM_PIDIV2;
	}
	else if (notation == "Bi") {
		setPieceEntityPivot('z', -.5f);
		rotationTarget.z = XM_PIDIV2;
	}


	if (notation == "M") {
		setPieceEntityPivotMiddle('x');
	    rotationTarget.x = XM_PIDIV2;
	}
	else if (notation == "Mi") {
		setPieceEntityPivotMiddle('x');
		rotationTarget.x = -XM_PIDIV2;
	}
	else if (notation == "E") {
		setPieceEntityPivotMiddle('y');
		rotationTarget.y = -XM_PIDIV2;
	}
	else if (notation == "Ei") {
		setPieceEntityPivotMiddle('y');
		rotationTarget.y = XM_PIDIV2;
	}
	else if (notation == "S") {
		setPieceEntityPivotMiddle('z');
		rotationTarget.z = XM_PIDIV2;
	}
	else if (notation == "Si") {
		setPieceEntityPivotMiddle('z');
		rotationTarget.z = -XM_PIDIV2;
	}


	
	if (notation == "r") {
		setPieceEntityPivotDouble('x', 0.5f);
		rotationTarget.x = -XM_PIDIV2;
	}
	else if (notation == "l") {
		setPieceEntityPivotDouble('x', -0.5f);
		rotationTarget.x = XM_PIDIV2;
	}
	else if (notation == "u") {
		setPieceEntityPivotDouble('y', -.5f);
		rotationTarget.y = XM_PIDIV2;
	}
	else if (notation == "f") {
		setPieceEntityPivotDouble('z', -.5f);
		rotationTarget.z = XM_PIDIV2;
	}
	else if (notation == "d") {
		setPieceEntityPivotDouble('y', .5f);
		rotationTarget.y = -XM_PIDIV2;
	}
	else if (notation == "b") {
		setPieceEntityPivotDouble('z', .5f);
		rotationTarget.z = -XM_PIDIV2;
	}


	if (notation == "ri") {
		setPieceEntityPivotDouble('x', 0.5f);
		rotationTarget.x = XM_PIDIV2;
	}
	else if (notation == "li") {
		setPieceEntityPivotDouble('x', -0.5f);
		rotationTarget.x = -XM_PIDIV2;
	}
	else if (notation == "ui") {
		setPieceEntityPivotDouble('y', -.5f);
		rotationTarget.y = -XM_PIDIV2;
	}
	else if (notation == "fi") {
		setPieceEntityPivotDouble('z', -.5f);
		rotationTarget.z = -XM_PIDIV2;
	}
	else if (notation == "di") {
		setPieceEntityPivotDouble('y', .5f);
		rotationTarget.y = XM_PIDIV2;
	}
	else if (notation == "bi") {
		setPieceEntityPivotDouble('z', .5f);
		rotationTarget.z = XM_PIDIV2;
	}



	if (notation == "X") {
		setPieceEntityPivotAll();
		rotationTarget.x = -XM_PIDIV2;
	}
	else if (notation == "Xi") {
		setPieceEntityPivotAll();
		rotationTarget.x = XM_PIDIV2;
	}
	else if (notation == "Y") {
		setPieceEntityPivotAll();
		rotationTarget.y = XM_PIDIV2;
	}
	else if (notation == "Z") {
		setPieceEntityPivotAll();
		rotationTarget.z = XM_PIDIV2;
	}
	else if (notation == "Yi") {
		setPieceEntityPivotAll();
		rotationTarget.y = -XM_PIDIV2;
	}
	else if (notation == "Zi") {
		setPieceEntityPivotAll();
		rotationTarget.z = -XM_PIDIV2;
	}


	rotVelocity = { rotationTarget.x * 4.f, rotationTarget.y * 4.f, rotationTarget.z * 4.f };
	
	OutputDebugStringA("H");
	//cubeRotInMotion = true;
}


void GameplaySystem::setPieceEntityPivot(const char axis, const float cmpPos) {
	entitiesPiecesInRot.clear();

	float pos = 0.f;
	for (auto epiece : entitiesPiece) {
		auto& ctransPiece = registry->getComponent<CTransform>(epiece);

		// get translation from model matrix; normalize and compare
		if (axis == 'x')
			pos = ctransPiece.mxlocal._41;
		else if (axis == 'y')
			pos = ctransPiece.mxlocal._42;
		else
			pos = ctransPiece.mxlocal._43;

		// set pivot as parent of piece that will rotate 
		if (pos == cmpPos) {
			auto& chrchy = registry->getComponent<CHierarchy>(epiece);
			chrchy.entityParent = entityCntlPivot;
			entitiesPiecesInRot.push_back(epiece);
		}
	}

	cubeRotInMotion = true;
}

void GameplaySystem::setPieceEntityPivotMiddle(const char axis) {
	entitiesPiecesInRot.clear();

	float pos = 0.f;
	for (auto epiece : entitiesPiece) {
		auto& ctransPiece = registry->getComponent<CTransform>(epiece);

		// get translation from model matrix; normalize and compare
		if (axis == 'x')
			pos = ctransPiece.mxlocal._41;
		else if (axis == 'y')
			pos = ctransPiece.mxlocal._42;
		else
			pos = ctransPiece.mxlocal._43;

		// set pivot as parent of piece that will rotate 
		if (pos == 0.f) {
			auto& chrchy = registry->getComponent<CHierarchy>(epiece);
			chrchy.entityParent = entityCntlPivot;
			entitiesPiecesInRot.push_back(epiece);
		}
	}

	cubeRotInMotion = true;

}

void GameplaySystem::setPieceEntityPivotDouble(const char axis, const float cmpPos) {
	entitiesPiecesInRot.clear();

	float pos = 0.f;
	for (auto epiece : entitiesPiece) {
		auto& ctransPiece = registry->getComponent<CTransform>(epiece);

		// get translation from model matrix; normalize and compare
		if (axis == 'x')
			pos = ctransPiece.mxlocal._41;
		else if (axis == 'y')
			pos = ctransPiece.mxlocal._42;
		else
			pos = ctransPiece.mxlocal._43;

		// set pivot as parent of piece that will rotate 
		if (pos != cmpPos) {
			auto& chrchy = registry->getComponent<CHierarchy>(epiece);
			chrchy.entityParent = entityCntlPivot;
			entitiesPiecesInRot.push_back(epiece);
		}
	}

	cubeRotInMotion = true;

}

void GameplaySystem::setPieceEntityPivotAll() {
	entitiesPiecesInRot.clear();

	float pos = 0.f;
	for (auto epiece : entitiesPiece) {
		auto& chrchy = registry->getComponent<CHierarchy>(epiece);
		chrchy.entityParent = entityCntlPivot;
		entitiesPiecesInRot.push_back(epiece);
	}

	cubeRotInMotion = true;

}


void GameplaySystem::resetCentralPivot() {
	// reset pivot
	auto& cTrans = registry->getComponent<CTransform>(entityCntlPivot);
	cTrans.rot = { 0.f, 0.f, 0.f };

	// model matrix = scale * rot * trans
	auto mxmodel = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&cTrans.rot)) * XMMatrixTranslationFromVector(XMLoadFloat3(&cTrans.pos));
	XMStoreFloat4x4(&cTrans.mxmodel, mxmodel);
}


void GameplaySystem::raycastPick(const int sx, const int sy, XMFLOAT4X4 mproj, XMFLOAT4X4 mview) {
	// Compute picking ray in view space.
	float vx = (2.0f * sx / mWndWidth - 1.0f) / mproj(0, 0);
	float vy = (-2.0f * sy / mWndHeight + 1.0f) / mproj(1, 1);

	// Ray definition in view space.
	XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);

	XMMATRIX xmview = XMLoadFloat4x4(&mview);
	auto xmviewDet = XMMatrixDeterminant(xmview);
	XMMATRIX invView = XMMatrixInverse(&xmviewDet, xmview);

	for (auto& e : entitiesPiece) {
		auto& cbbox = registry->getComponent<CBoundingBox>(e);
		auto& ctrans = registry->getComponent<CTransform>(e);

		XMMATRIX xmmodel = XMLoadFloat4x4(&ctrans.mxmodel);
		auto xmmodelDet = XMMatrixDeterminant(xmmodel);
		XMMATRIX invModel = XMMatrixInverse(&xmmodelDet, xmmodel);

		// convert ray to local space of the model
		XMMATRIX toLocal = XMMatrixMultiply(invView, invModel);

		rayOrigin = XMVector3TransformCoord(rayOrigin, toLocal);
		rayDir = XMVector3TransformNormal(rayDir, toLocal);

		// normalize to get true direction
		rayDir = XMVector3Normalize(rayDir);


		float rayDist = 150.f;
		if (cbbox.obb.Intersects(rayOrigin, rayDir, rayDist)) {
			OutputDebugStringA("\n\n\n\n\nRay hit!!!\n\n\n\n\n\n");
		}

	}

}
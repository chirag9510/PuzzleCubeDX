#pragma once
#include "stdafx.h"
#include "Registry.h"

#include <vector>
#include <map>
#include <queue>

class GameplaySystem {
public:
	GameplaySystem();
	void onInit(std::shared_ptr<Registry> registry, int mWndWidth, int mWndHeight);
	bool onUpdate(const float& deltaTime);
	void onReset();
	void onShuffle();

	void processInputCmd(const std::string strcmd);
	void raycastPick(const int sx, const int sy, XMFLOAT4X4 mproj, XMFLOAT4X4 mview);

	std::vector<std::string> cubeNotations;

private:

	void rotateCubeSide(std::string notation);														// rotate cube

	void storeEntities();
	void createCubeNotations();
	void setPieceEntityPivot(const char axis, const float cmpPos);
	void setPieceEntityPivotMiddle(const char axis);
	void setPieceEntityPivotDouble(const char axis, const float cmpPos);
	void setPieceEntityPivotAll();
	void resetCentralPivot();

	std::shared_ptr<Registry> registry;
	int mWndWidth, mWndHeight;

	bool cubeRotInMotion, rotTargetReached;
	XMFLOAT3 rotationTarget;

	UINT entityCntlPivot;													//Central pivot entity
	std::vector<UINT> entitiesFace;										//CFace face mesh entities
	std::vector<UINT> entitiesPiece;										//CPiece piece mesh entities
	std::vector<UINT> entitiesPiecesInRot;									// pieces that rotate around pivot

	std::map<UINT, std::vector<XMFLOAT3>> faceDirPositions;

	std::queue<std::string> queueCmd;
	XMFLOAT3 rotVelocity, currRotation;

};



#pragma once
#include "stdafx.h"
#include "Registry.h"

#include <vector>
#include <map>

class GameplaySystem {
public:
	void onInit(std::shared_ptr<Registry> registry);
	void onUpdate();
	void onReset();

private:

	void storeEntities();

	std::shared_ptr<Registry> registry;

	std::vector<UINT> entitiesFace;										//CFace face mesh entities

	std::map<UINT, std::vector<XMFLOAT3>> faceDirPositions;
};



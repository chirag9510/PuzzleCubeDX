// pso and shader processing class
#pragma once
#include "stdafx.h"
#include "Registry.h"


#include <map>

struct MeshData {
	// these 2 objects store the vertex and index raw data
	std::vector<float> vertexData;
	std::vector<uint16_t> indexData;
};

class LevelLoader {
public:
	void loadLevel(std::shared_ptr<Registry> registry);

	std::vector<float> vertBuffData;
	std::vector<uint16_t> indexBuffData;

private:
	void createEntities();
	CDraw& setEntityDraw(UINT& entity, std::string strFilename, bool createOBB = false);
	void assembleLevel();

	void createPieceEntity(PieceType pieceType);
	UINT createFaceEntity(const UINT ePiece, std::string strModelFile, XMFLOAT3 translation, XMFLOAT3 rot = XMFLOAT3(0.f, 0.f, 0.f));

	std::shared_ptr<Registry> registry;

	std::map<std::string, MeshData> mapModelData;

	// entities based on piece type
	std::map<PieceType, std::vector<UINT>> entitiesPieces;

	UINT drawIndex = 0, baseVertex = 0, startIndex = 0;

};


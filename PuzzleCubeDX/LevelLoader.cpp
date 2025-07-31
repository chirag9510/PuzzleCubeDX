#include "LevelLoader.h"
#include <iostream>

#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#endif
#include "tiny_obj_loader.h"


// cubes have 0.5 edge lengths
XMFLOAT3 centerPiecesLoc[]{
	{0.f, 0.f, 0.5f},						
	{0.5f, 0.f, 0.f},						
	{0.f, 0.f, -0.5f},						
	{-0.5f, 0.f, 0.f},					
	{0.f, 0.5f, 0.f},						
	{0.f, -0.5f, 0.f}
};
XMFLOAT3 centerPiecesRot[]{
	{0.f, 0.f, 0.f},
	{0.f, XM_PIDIV2, 0.f},
	{0.f, XM_PI, 0.f},
	{0.f, XM_PI + XM_PIDIV2, 0.f},
	{XM_PI + XM_PIDIV2, 0.f, 0.f},
	{XM_PIDIV2, 0.f, 0.f}
};

XMFLOAT3 cornerPiecesLoc[]{
	{-0.5f, 0.5f, 0.5f},
	{-0.5f, -0.5f, 0.5f},
	{0.5f, -0.5f, 0.5f},
	{0.5f, 0.5f, 0.5f},

	{-0.5f, 0.5f, -0.5f},
	{-0.5f, -0.5f, -0.5f},
	{0.5f, -0.5f, -0.5f},
	{0.5f, 0.5f, -0.5f}
};
XMFLOAT3 cornerPiecesRot[]{
	{0.f, 0.f, 0.f},
	{0.f, 0.f, XM_PIDIV2},
	{0.f, 0.f, XM_PI},
	{0.f, 0.f, XM_PI + XM_PIDIV2},

	{0.f, XM_PI + XM_PIDIV2, 0.f},
	{0.f, XM_PI + XM_PIDIV2, XM_PIDIV2},
	{0.f, XM_PIDIV2, XM_PI},
	{0.f, XM_PIDIV2, XM_PI + XM_PIDIV2}
};

XMFLOAT3 crossPiecesLoc[]{
	{0.f, 0.5f, 0.5f},
	{-0.5f, 0.f, 0.5f},
	{0.f, -0.5f, 0.5f},
	{0.5f, 0.f, 0.5f},

	{0.f, 0.5f, -0.5f},
	{-0.5f, 0.f, -0.5f},
	{0.f, -0.5f, -0.5f},
	{0.5f, 0.f, -0.5f},

	{-0.5f, 0.5f, 0.f},
	{0.5f, 0.5f, 0.f},
	{-0.5f, -0.5f, 0.f},
	{0.5f, -0.5f, 0.f},

};

XMFLOAT3 crossPiecesRot[]{
	{0.f, 0.f, 0.f},
	{0.f, 0.f, XM_PIDIV2},
	{0.f, 0.f, XM_PI},
	{0.f, 0.f, XM_PI + XM_PIDIV2},

	{0.f, XM_PI, 0.f},
	{0.f, XM_PIDIV2 + XM_PI, XM_PIDIV2},
	{0.f, XM_PI, XM_PI},
	{0.f, XM_PIDIV2, XM_PI + XM_PIDIV2},

	{0.f, XM_PIDIV2 + XM_PI, 0.f},
	{0.f, XM_PIDIV2, 0.f},
	{0.f, XM_PIDIV2 + XM_PI, XM_PI},
	{0.f, XM_PIDIV2, XM_PI},

};

void LevelLoader::loadLevel(std::shared_ptr<Registry> registry) {
	// construct the cube
	this->registry = registry;

	createEntities();
	assembleLevel();
}


void LevelLoader::assembleLevel() {
	UINT i = 0;
	for (auto& e : entitiesPieces[PieceType::PIECE_CORNER]) {
		auto& ctrans = registry->getComponent<CTransform>(e);
		ctrans.pos = cornerPiecesLoc[i];
		ctrans.rot = cornerPiecesRot[i];
		i++;
	}
	i = 0;
	for (auto& e : entitiesPieces[PieceType::PIECE_CENTER]) {
		auto& ctrans = registry->getComponent<CTransform>(e);
		ctrans.pos = centerPiecesLoc[i];
		ctrans.rot = centerPiecesRot[i];
		i++;
	}
	i = 0;
	for (auto& e : entitiesPieces[PieceType::PIECE_CROSS]) {
		auto& ctrans = registry->getComponent<CTransform>(e);
		ctrans.pos = crossPiecesLoc[i];
		ctrans.rot = crossPiecesRot[i];
		i++;
	}
}


void LevelLoader::createEntities() {
	// load pivots
	auto e = registry->createEntity();
	registry->addComponent<CCentralPivot>(e);

	for (int i = 0; i < 6; i++) {
		e = registry->createEntity();
		registry->addComponent<CPivot>(e);
		auto& transform = registry->addComponent<CTransform>(e);
	}

	// load pieces and thier faces
	for (int i = 0; i < 8; i++)
		createPieceEntity(PieceType::PIECE_CORNER);
	for (int i = 0; i < 6; i++)
		createPieceEntity(PieceType::PIECE_CENTER);
	for (int i = 0; i < 12; i++)
		createPieceEntity(PieceType::PIECE_CROSS);
	
}

void LevelLoader::createPieceEntity(PieceType pieceType) {
	// create pieces and faces without any world space translation. that comes later when assembling the cube
	auto ePiece = registry->createEntity();
	auto& hierrPiece = registry->addComponent<CHierarchy>(ePiece);									// each piece has multiple child faces entities in its heirarchy
	registry->addComponent<CPiece>(ePiece);
	registry->addComponent<CTransform>(ePiece);
	CDraw& cdraw = setEntityDraw(ePiece, "Piece.obj");
	cdraw.colorIndex = 6;														// 7th color is the piece color


	switch (pieceType) {
	case PieceType::PIECE_CENTER:
	{

		hierrPiece.childEntities.push_back(
			createFaceEntity(ePiece, "Face_center.obj", XMFLOAT3(0.f, 0.f, 0.25f))
		);
	}

		break;
	case PieceType::PIECE_CROSS:
	{
			// 2 faces on cross, back and top
		hierrPiece.childEntities.push_back(
			createFaceEntity(ePiece, "Face_cross.obj", XMFLOAT3(0.f, 0.f, 0.25f))
		);
		hierrPiece.childEntities.push_back(
			createFaceEntity(ePiece, "Face_cross.obj", XMFLOAT3(0.f, 0.25f, 0.f), XMFLOAT3(XM_PIDIV2 + XM_PI, 0.f, XM_PI))
		);
	}

		break;

	case PieceType::PIECE_CORNER:
	{
		// 3 faces on cross, back and left
		hierrPiece.childEntities.push_back(
			createFaceEntity(ePiece, "Face_corner.obj", XMFLOAT3(0.f, 0.f, 0.25f))			// front center, facing the back of piece since forward vector will be 0,0,1
		);
		hierrPiece.childEntities.push_back(
			createFaceEntity(ePiece, "Face_corner.obj", XMFLOAT3(0.f, 0.25f, 0.f), XMFLOAT3(XM_PIDIV2 + XM_PI, 0.f, 0.f))	// top
		);
		hierrPiece.childEntities.push_back(
			createFaceEntity(ePiece, "Face_corner.obj", XMFLOAT3(-0.25f, 0.f, 0.f), XMFLOAT3(0.f, -XM_PIDIV2, 0.f))    // left
		);
	}
		break;
	}


	// store for cube assembly later
	entitiesPieces[pieceType].push_back(ePiece);
}

UINT LevelLoader::createFaceEntity(const UINT ePiece, std::string strModelFile, XMFLOAT3 translation, XMFLOAT3 rot) {
	auto eface = registry->createEntity();
	registry->addComponent<CFace>(eface);
	auto& trans = registry->addComponent<CTransform>(eface);
	trans.pos = translation;										// front center, facing the back of piecesince forward vector will be 0,0,1
	trans.rot = rot;

	registry->addComponent<CHierarchy>(eface, ePiece);
	setEntityDraw(eface, strModelFile);

	return eface;
}


CDraw& LevelLoader::setEntityDraw(UINT& entity, std::string strFilename) {
	// store data from obj file in map and use it again if already loaded instead of loading model again 
	if (mapModelData.find(strFilename) == mapModelData.end()) {
	
		tinyobj::ObjReaderConfig readerConfig;
		readerConfig.mtl_search_path = "./";
		readerConfig.triangulate = true;
		tinyobj::ObjReader reader;
		
		//TODO: these error messages
		std::string strFilePath = "./Assets/" + strFilename;
		if (!reader.ParseFromFile(strFilePath, readerConfig)) {
			if (!reader.Error().empty()) {
				std::cerr << "TinyObjReader: " << reader.Error() << std::endl;
			}
		}
		if (!reader.Warning().empty()) {
			std::cerr << "TinyObjReader: " << reader.Warning() << std::endl;
		}

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		
		// only processing and sending position data to shader for now. not processing normals
		mapModelData[strFilename].vertexData = attrib.vertices;		
		for (size_t s = 0; s < shapes.size(); s++) {
			std::vector<uint16_t> indices;
			for(auto& index : shapes[s].mesh.indices)
				indices.push_back(index.vertex_index);
			mapModelData[strFilename].indexData.insert(mapModelData[strFilename].indexData.end(), indices.begin(), indices.end());
		}
	}

	vertBuffData.insert(vertBuffData.end(), mapModelData[strFilename].vertexData.begin(), mapModelData[strFilename].vertexData.end());
	indexBuffData.insert(indexBuffData.end(), mapModelData[strFilename].indexData.begin(), mapModelData[strFilename].indexData.end());

	// add draw comp to the currently held entity
	CDraw& cdraw = registry->addComponent<CDraw>(entity, drawIndex++, baseVertex, startIndex, mapModelData[strFilename].indexData.size());

	// baseVertex will require num vertices. vertexData is full float sequence of x,y,z. To get num vertices, vertexData / 3 (3 = float3(x,y,z)) 
	baseVertex += (mapModelData[strFilename].vertexData.size() / 3);
	startIndex += mapModelData[strFilename].indexData.size();
	

	return cdraw;
}

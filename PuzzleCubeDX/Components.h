#pragma once
#include "stdafx.h"

using namespace DirectX;

struct CBuffPerObj {
	DirectX::XMFLOAT4X4 matModel;
	UINT colorIndex;
};

struct CBuffPerPass {
	DirectX::XMFLOAT4X4 matView;
	DirectX::XMFLOAT4X4 matProj;
};

enum class PieceType {
	PIECE_CORNER,
	PIECE_CROSS,
	PIECE_CENTER,
	PIECETYPE_TOTAL
};

struct CDraw {
	CDraw(UINT drawIndex = 0, UINT baseVertex = 0, UINT startIndex = 0, UINT indexCount = 0) :
		drawIndex(drawIndex), baseVertex(baseVertex), startIndex(startIndex), indexCount(indexCount), colorIndex(0) {}
	UINT drawIndex;
	UINT baseVertex;
	UINT startIndex;
	UINT indexCount;
	UINT colorIndex;
};

struct CTransform {
	XMFLOAT3 pos;								// world position
	XMFLOAT3 forward;							// forward facing vector of the object
	XMFLOAT3 rot;								// xyz rotation 

	XMFLOAT4X4 mxmodel;							// final model matrix

	CTransform() : pos(0.f, 0.f, 0.f), forward(0.f, 0.f, 1.f) {
		XMStoreFloat4x4(&mxmodel, XMMatrixIdentity());
	}
};

struct CHierarchy {												
	CHierarchy(int entityParent = -1) : entityParent(entityParent) {}			//-1 means it has no parent; may have children
	int entityParent;
	std::vector<UINT> childEntities;
};

struct CFace {
	XMFLOAT3 color;
};

struct CPiece {};
struct CPiece_Corner{};
struct CPiece_Cross{};
struct CPiece_Center{};

struct CPivot {};

struct CCentralPivot {};
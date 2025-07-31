#pragma once
#include "stdafx.h"
#include "Registry.h"


class RenderSystem {
public:
	void onInit(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::shared_ptr<Registry> registry, const float aspectRatio, std::vector<float>& vertices, std::vector<uint16_t>& indices);
	void onUpdate();
	void onUpdateView(const float& radius, const float& theta, const float& phi);
	void onDraw(ID3D12GraphicsCommandList* commandList);

	ID3D12RootSignature* getRootSignature();
	UINT getNumCBufferDescriptors();


private:
	void storeDrawableEntities();
	void createConstantBuffers(ID3D12Device* device);
	void initBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<float>& vertices, std::vector<uint16_t>& indices);
	ComPtr<ID3D12Resource> loadBufferDataIntoDefaultHeap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* bufferData, UINT64 bufferByteSize, ComPtr<ID3D12Resource>& resUploadBuffer);
	void createRootSignature(ID3D12Device* device);

	float roundoff(float ff);

	std::shared_ptr<Registry> registry;

	ComPtr<ID3D12RootSignature> mRootSignature;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap;
	UINT mCbvSrvDescriptorSize;
	ComPtr<ID3D12Resource> resCBPerObj, resCBPerPass;
	BYTE *pCBPerPass, *pCBPerObj;												// constant buffer pointer for draw index at b1
	UINT numCBuffDescriptors;

	XMFLOAT4X4 mproj;

	ComPtr<ID3D12Resource> vertBuffDefault, vertBuffUpload;
	ComPtr<ID3D12Resource> indexBuffDefault, indexBuffUpload;

	D3D12_VERTEX_BUFFER_VIEW vertBuffView;
	D3D12_INDEX_BUFFER_VIEW indexBuffView;

	ID3D12GraphicsCommandList* commandList;

	// entities that have CDraw. useful for quick lookup
	std::vector<UINT> entitiesDraw;
};



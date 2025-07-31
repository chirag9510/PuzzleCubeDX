#include "RenderSystem.h"
#include "Helper.h"

using namespace DirectX;


float angle = 0.f;

void RenderSystem::onUpdate() {
	// update all model matrices of every piece and face
	// update perobj cbuffer with all entities model matrix
	XMMATRIX mxmodel, mxlocal;
	std::vector<CBuffPerObj> cbPerObjects;
	int i = 0;
	auto cbObjSize = calcConstantBufferByteSize(sizeof(CBuffPerObj));
	for (auto& e : entitiesDraw) {
		auto& cTransform = registry->getComponent<CTransform>(e);
		auto cDraw = registry->getComponent<CDraw>(e);
		auto& cHrchy = registry->getComponent<CHierarchy>(e);


		// full hierarical relationship final position
		if (cHrchy.entityParent != -1) {
			XMMATRIX scale = XMMatrixScaling(0.9, 0.9, 1.5);

			// model matrix = scale * rot * trans
			mxlocal = scale * XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&cTransform.rot)) * XMMatrixTranslationFromVector(XMLoadFloat3(&cTransform.pos));

			
			auto& transParent = registry->getComponent<CTransform>(cHrchy.entityParent);

			// final position = local mat * parent mat 
			mxmodel =  mxlocal * XMLoadFloat4x4(&transParent.mxmodel);
		}
		else 
		{
			XMMATRIX scale = XMMatrixScaling(0.97, 0.97, 0.97);

			// model matrix = scale * rot * trans
			mxlocal = scale * XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&cTransform.rot)) * XMMatrixTranslationFromVector(XMLoadFloat3(&cTransform.pos));

			mxmodel = mxlocal;
		}
		

		XMStoreFloat4x4(&cTransform.mxmodel, mxmodel);

		CBuffPerObj cbObj;
		XMStoreFloat4x4(&cbObj.matModel, XMMatrixTranspose(mxmodel));							// remember to transpose
		cbObj.colorIndex = cDraw.colorIndex;

		cTransform.forward = XMFLOAT3(
			roundoff(cbObj.matModel._13),
			roundoff(cbObj.matModel._23),
			roundoff(cbObj.matModel._33));

		memcpy(&pCBPerObj[i * cbObjSize], &cbObj, sizeof(CBuffPerObj));
		i++;
	}
}

void RenderSystem::onUpdateView(const float& radius, const float& theta, const float& phi) {
	// Convert Spherical to Cartesian coordinates for camera
	float x = radius * sinf(phi) * cosf(theta);
	float z = radius * sinf(phi) * sinf(theta);
	float y = radius * cosf(phi);

	// fixed view for now
	XMVECTOR pos = XMVectorSet(x, y, z, 1.f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX mview = XMMatrixLookAtLH(pos, target, up);
	

	CBuffPerPass cbufViewProj;
	XMStoreFloat4x4(&cbufViewProj.matView, XMMatrixTranspose(mview));					// remember to transpose
	XMStoreFloat4x4(&cbufViewProj.matProj, XMMatrixTranspose(XMLoadFloat4x4(&mproj)));

	memcpy(pCBPerPass, &cbufViewProj, sizeof(CBuffPerPass));
}

void RenderSystem::onDraw(ID3D12GraphicsCommandList* commandList) {
	// bind cbv to shader
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	
	commandList->SetGraphicsRootSignature(mRootSignature.Get());

	// bind perPass
	commandList->SetGraphicsRootConstantBufferView(1, resCBPerPass->GetGPUVirtualAddress());

	//// and bind to descriptortable for root signature
	//commandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertBuffView);
	commandList->IASetIndexBuffer(&indexBuffView);

	// loop over entities to draw
	CDraw cdraw;
	auto cbObjByteSize = calcConstantBufferByteSize(sizeof(pCBPerObj));
	UINT i = 0;
	for (auto& enttDraw : entitiesDraw) {
		cdraw = registry->getComponent<CDraw>(enttDraw);

		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = resCBPerObj->GetGPUVirtualAddress();
		cbAddress += (i * cbObjByteSize);
		
		commandList->SetGraphicsRootConstantBufferView(0, cbAddress);

		commandList->DrawIndexedInstanced(cdraw.indexCount, 1, cdraw.startIndex, cdraw.baseVertex, 0);
		
		i++;
	}
}

void RenderSystem::onInit(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::shared_ptr<Registry> registry, const float aspectRatio, std::vector<float>& vertices, std::vector<uint16_t>& indices) {
	this->registry = registry;

	initBuffers(device, cmdList, vertices, indices);
	storeDrawableEntities();
	createConstantBuffers(device);
	createRootSignature(device);

	XMStoreFloat4x4(&mproj, XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, aspectRatio, 1.f, 1000.f));

	// update the constant buffer matrices 
	onUpdate();
}

void RenderSystem::storeDrawableEntities() {
	// create signature for filtering entities with CDraw comp
	Signature signature;
	signature.set(registry->getComponentTypeID<CDraw>());

	// store all drawable entities that have CDraw
	entitiesDraw = registry->getEntitiesFromSignature(signature);
}

void RenderSystem::createConstantBuffers(ID3D12Device* device) {
	mCbvSrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	size_t numDrawEntities = entitiesDraw.size();

	// descriptors for all drawable entities perObject and 1 (at last pos) for perPass cbuffer
	numCBuffDescriptors = numDrawEntities + 1;

	// cbuffer descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc;
	cbHeapDesc.NumDescriptors = numCBuffDescriptors;
	cbHeapDesc.NodeMask = 0;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(
		device->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(mCbvHeap.GetAddressOf()))
	);

	// first perobject, create cbv for each entity
	// create buffer resource first
	auto cbObjSize = calcConstantBufferByteSize(sizeof(CBuffPerObj));
	auto cbuffSize = cbObjSize * numDrawEntities;
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(cbuffSize);
	ThrowIfFailed(
		device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&resCBPerObj))
	);
	// data mapping pointer
	resCBPerObj->Map(0, nullptr, reinterpret_cast<void**>(&pCBPerObj));

	// now create cbv for each entity perobject data
	for (UINT i = 0; i < numDrawEntities; i++) {

		// get pointer at offset to the ith position inside the constant buffer resource
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = resCBPerObj->GetGPUVirtualAddress();
		cbAddress += (i * cbObjSize);

		// get handle at ith position from the cbv heap
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(i, mCbvSrvDescriptorSize);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = cbObjSize;
		device->CreateConstantBufferView(&cbvDesc, handle);
	}

	
	// 2nd, the perPass cbuffer
	cbuffSize = calcConstantBufferByteSize(sizeof(CBuffPerPass));
	heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(cbuffSize);
	ThrowIfFailed(
		device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			0, IID_PPV_ARGS(&resCBPerPass))
	);
	resCBPerPass->Map(0, nullptr, reinterpret_cast<void**>(&pCBPerPass));

	// handle at the last position on heap
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
	handle.Offset(numDrawEntities, mCbvSrvDescriptorSize);

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = resCBPerPass->GetGPUVirtualAddress();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbPerPassDesc;
	cbPerPassDesc.BufferLocation = cbAddress;
	cbPerPassDesc.SizeInBytes = cbuffSize;
	device->CreateConstantBufferView(&cbPerPassDesc, handle);
}

void RenderSystem::createRootSignature(ID3D12Device* device) {
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];

	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//TOEDIT
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
	if (errorBlob != nullptr) {
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);


	ThrowIfFailed(
		device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&mRootSignature))
	);
}

void RenderSystem::initBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<float>& vertices, std::vector<uint16_t>& indices) {
	// init default heap buffers and views
	const UINT64 vertBufferSize = sizeof(float) * vertices.size();
	const UINT64 indexBufferSize = sizeof(std::uint16_t) * indices.size();

	vertBuffDefault = loadBufferDataIntoDefaultHeap(device, cmdList, vertices.data(), vertBufferSize, vertBuffUpload);

	indexBuffDefault = loadBufferDataIntoDefaultHeap(device, cmdList, indices.data(), indexBufferSize, indexBuffUpload);

	vertBuffView.BufferLocation = vertBuffDefault->GetGPUVirtualAddress();
	vertBuffView.SizeInBytes = vertBufferSize;
	vertBuffView.StrideInBytes = sizeof(float) * 3;								// only position data of vector3 for now to send to shader

	indexBuffView.BufferLocation = indexBuffDefault->GetGPUVirtualAddress();
	indexBuffView.Format = DXGI_FORMAT_R16_UINT;
	indexBuffView.SizeInBytes = indexBufferSize;
}

ComPtr<ID3D12Resource> RenderSystem::loadBufferDataIntoDefaultHeap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* bufferData, UINT64 bufferByteSize, ComPtr<ID3D12Resource>& resUploadBuffer) {
	ComPtr<ID3D12Resource> resDefaultBuffer;

	// TODO: set resource state directly to copy destination
	// default heap resource
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferByteSize);
	ThrowIfFailed(
		device->CreateCommittedResource(
			&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
			D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(resDefaultBuffer.GetAddressOf()))
	);

	// intermediate upload heap resource
	heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(
		device->CreateCommittedResource(
			&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(resUploadBuffer.GetAddressOf()))
	);

	// data type to be copied into default buffer resource
	D3D12_SUBRESOURCE_DATA subData = {};
	subData.pData = bufferData;
	subData.RowPitch = bufferByteSize;
	subData.SlicePitch = subData.RowPitch;


	// default buff resource transition into copy destination
	auto resBarrierTransition = CD3DX12_RESOURCE_BARRIER::Transition(resDefaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	cmdList->ResourceBarrier(1, &resBarrierTransition);
	
	UpdateSubresources<1>(cmdList, resDefaultBuffer.Get(), resUploadBuffer.Get(), 0, 0, 1, &subData);

	resBarrierTransition = CD3DX12_RESOURCE_BARRIER::Transition(resDefaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList->ResourceBarrier(1, &resBarrierTransition);

	return resDefaultBuffer;

}


float RenderSystem::roundoff(float val) {
	if ((val > 0.f && val < 0.1f) || (val < 0.f && val > -0.1f)) {
		return 0.f;
	}
	else if (val > 0.f)
		return 1.f;
	else if (val < 0.f)
		return -1.f;
	else
		return 0.f;
}


ID3D12RootSignature* RenderSystem::getRootSignature() {
	return mRootSignature.Get();
}

UINT RenderSystem::getNumCBufferDescriptors() {
	return numCBuffDescriptors;
}
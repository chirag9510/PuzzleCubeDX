#include "Core.h"
#include "Helper.h"

#include <windowsx.h>

using namespace DirectX;

Core::Core(int wndWidth, int wndHeight) : mWndWidth(wndWidth), mWndHeight(wndHeight), 
	mMinimized(false), mMaximized(false),
	mUseMSAA(false), mMSAAQualityLevel(0),
	mCurrentFence(0)
{
	levelLoader = std::make_unique<LevelLoader>();
	registry = std::make_shared<Registry>();

}

void Core::initialize() {
	initD3D12Device();
	createSwapChain();

	
	ThrowIfFailed(mCommandList->Reset(mCmdAlloc.Get(), nullptr));

	createRTandDSViews();

	loadAssets();
	loadPipeline();

	//done recording
	mCommandList->Close();
	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCmdQueue->ExecuteCommandLists(1, cmdLists);
	flushCommandQueue();

	//// free up cpu vertex data memory since its gone to gpu
	//levelLoader.reset();
}

LRESULT Core::run(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wparam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wparam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wparam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		return 0;

	case WM_SIZE:
		// save new window dimensions after resize
		mWndWidth = LOWORD(lparam);
		mWndHeight = HIWORD(lparam);
		if (mDevice)
		{
			if (wparam == SIZE_MAXIMIZED) {
				mMinimized = false;
				mMaximized = true;
				onResize();
			}
			else if (wparam == SIZE_RESTORED) {

				// Restoring from minimized state?
				if (mMinimized) {
					mMinimized = false;
					onResize();
				}

				// Restoring from maximized state?
				else if (mMaximized) {
					mMaximized = false;
					onResize();
				}
				else {												// API call such as SetWindowPos or mSwapChain->SetFullscreenState.
					onResize();
				}
			}
		}
		return 0;
	
	case WM_PAINT:
		onUpdate();
		onRender();

		return 0;


	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}
void Core::OnMouseDown(WPARAM btnState, int x, int y) {
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(hwnd);

	mUpdateView = true;
}

void Core::OnMouseUp(WPARAM btnState, int x, int y) {
	ReleaseCapture();
	mUpdateView = false;
}

void Core::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = std::clamp(mPhi, 0.1f, XM_PI - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = std::clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}


void Core::onUpdate() {	
	if (mUpdateView) 
		renderSystem.onUpdateView(mRadius, mTheta, mPhi);

}

void Core::onRender() {
	ThrowIfFailed(mCmdAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(mCmdAlloc.Get(), mPipelineState.Get()));

	mCommandList->RSSetViewports(1, &mViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// commands for clear the back buffer and depth buffer
	ID3D12Resource* backBuffer = mSwapChainBuffer[mCurrBackBuffer].Get();
	auto rtResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	auto backBufferView = getCurrentBackBufferView();
	auto depthBufferView = getDepthStencilView();

	const float colorbg[] = {0.56f, 0.f, 0.27f, 1.f};
	mCommandList->ResourceBarrier(1, &rtResourceBarrier);
	mCommandList->ClearRenderTargetView(backBufferView, colorbg, 0, nullptr);
	mCommandList->ClearDepthStencilView(depthBufferView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	mCommandList->OMSetRenderTargets(1, &backBufferView, true, &depthBufferView);

	renderSystem.onDraw(mCommandList.Get());

	rtResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mCommandList->ResourceBarrier(1, &rtResourceBarrier);

	
	//done recording
	mCommandList->Close();
	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCmdQueue->ExecuteCommandLists(1, cmdLists);


	ThrowIfFailed(mSwapChain->Present(1, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % mSwapChainBufferCount;

	flushCommandQueue();
}

void Core::onResize() {
	// if the window is resized, create new swap chain and buffers to updated window dimensions

	flushCommandQueue();
	ThrowIfFailed(mCmdAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(mCmdAlloc.Get(), nullptr));


	for (UINT i = 0; i < mSwapChainBufferCount; i++)
		mSwapChainBuffer[i].Reset();

	mDepthStencilBuffer.Reset();

	// Resize the swap chain buffers. Call this to avoid errors
	ThrowIfFailed(mSwapChain->ResizeBuffers(mSwapChainBufferCount, mWndWidth, mWndHeight, mBackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	// recreate both swapchain and the rtv dsv with new sizes
	createSwapChain();
	createRTandDSViews();

	// execute commands for buffer operations
	mCommandList->Close();
	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCmdQueue->ExecuteCommandLists(1, cmdLists);
	flushCommandQueue();


	mViewport.TopLeftX = 0.0f;
	mViewport.TopLeftY = 0.0f;
	mViewport.Width = static_cast<float>(mWndWidth);
	mViewport.Height = static_cast<float>(mWndHeight);
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mWndWidth, mWndHeight };
}

void Core::destroy() {

}

void Core::flushCommandQueue() {
	mCurrentFence++;
	ThrowIfFailed(mCmdQueue->Signal(mFence.Get(), mCurrentFence));

	if (mFence->GetCompletedValue() < mCurrentFence) {

		HANDLE eventHandle = CreateEventEx(nullptr, 0, 0, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));
		
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}


void Core::loadAssets() { 
	// order matters
	levelLoader->loadLevel(registry);												// load and assemble
	renderSystem.onInit(mDevice.Get(),												// create model matrices and get forward vector
		mCommandList.Get(), registry, 
		static_cast<float>(mWndWidth) / static_cast<float>(mWndHeight), 
		levelLoader->vertBuffData, levelLoader->indexBuffData); 
	gameplaySystem.onInit(registry);												// color the faces when after getting forward vectors


	// update at least once after load
	renderSystem.onUpdate();
	renderSystem.onUpdateView(mRadius, mTheta, mPhi);
}





void Core::loadPipeline() {
	// Compile shaders
#if defined(_DEBUG)
// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	D3D12_INPUT_ELEMENT_DESC vertexElementDesc[] = {
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;
	HRESULT hr = D3DCompileFromFile(L"./shaders/color.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
	if (FAILED(hr))
		MessageBox(0, L"Error: Shader Compile From File", 0, 0);

	hr = D3DCompileFromFile(L"./shaders/color.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);
	if (FAILED(hr))
		MessageBox(0, L"Error: Shader Compile From File", 0, 0);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { vertexElementDesc, _countof(vertexElementDesc) };
	psoDesc.pRootSignature = renderSystem.getRootSignature();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = mUseMSAA ? 4 : 1;
	psoDesc.SampleDesc.Quality = mUseMSAA ? (mMSAAQualityLevel - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;

	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState)));
}


void Core::initD3D12Device() {
	UINT dxgiFactoryFlags = 0;

#if defined(DEBUG) || defined(_DEBUG) 
	ComPtr<ID3D12Debug>	debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		// Enable additional debug layers.
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));

	//device
	auto result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice));
	// fallback to WARP if no device found
	if (FAILED(result)) {
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(mFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		//create device again with the WARP adapter this time
		ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)));
	}


	// fence
	ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

	// query descriptor increment sizes
	mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	

	// MSAA support level 4X
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.NumQualityLevels = 0;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	ThrowIfFailed(
		mDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels))
	);
	mMSAAQualityLevel = msQualityLevels.NumQualityLevels;


	// Command Objects
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(
		mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCmdQueue))
	);
	ThrowIfFailed(
		mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mCmdAlloc.GetAddressOf()))
	);
	ThrowIfFailed(
		mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAlloc.Get(), nullptr, IID_PPV_ARGS(mCommandList.GetAddressOf()))
	);
	// Close at first so reset can be called on it before we use it
	mCommandList->Close();


	// create Descriptor Heaps
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;							//rtv is not shader visible
	rtvHeapDesc.NumDescriptors = mSwapChainBufferCount;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(
		mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf()))
	);
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;							//rtv is not shader visible
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(
		mDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf()))
	);
}


void Core::createSwapChain() {
	// release prev swapchain
	mSwapChain.Reset();
	// reset back currentbackbuffer index since a new swapchain will be made
	mCurrBackBuffer = 0;

	DXGI_SWAP_CHAIN_DESC swapchainDesc = {0};
	swapchainDesc.BufferDesc.Width = mWndWidth;
	swapchainDesc.BufferDesc.Height = mWndHeight;
	swapchainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapchainDesc.BufferDesc.Format = mBackBufferFormat;
	swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapchainDesc.SampleDesc.Count = mUseMSAA ? 4 : 1;
	swapchainDesc.SampleDesc.Quality = mUseMSAA ? (mMSAAQualityLevel - 1) : 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.BufferCount = mSwapChainBufferCount;
	swapchainDesc.OutputWindow = hwnd;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	
	//windowed
	swapchainDesc.Windowed = true;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ThrowIfFailed(
		mFactory->CreateSwapChain(mCmdQueue.Get(), &swapchainDesc, mSwapChain.GetAddressOf())
	);

}

void Core::createRTandDSViews() {	
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < mSwapChainBufferCount; i++) {
		ThrowIfFailed(
			mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i]))
		);

		mDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);

		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	//create Depth Stencil View
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mWndWidth;
	depthStencilDesc.Height = mWndHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.SampleDesc.Count = mUseMSAA ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = mUseMSAA ? (mMSAAQualityLevel - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	//clear value
	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(
		mDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf()))
	);
	mDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, getDepthStencilView());

	CD3DX12_RESOURCE_BARRIER resbDepthWrite = CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	mCommandList->ResourceBarrier(1, &resbDepthWrite);
}



D3D12_CPU_DESCRIPTOR_HANDLE Core::getCurrentBackBufferView() {
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE Core::getDepthStencilView() {
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

ComPtr<ID3D12Resource> Core::getCurrentBackBuffer() const {
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}



int Core::getWndWidth() const {
	return mWndWidth;
}
int Core::getWndHeight() const {
	return mWndHeight;
}
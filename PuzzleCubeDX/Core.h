#pragma once
#include "stdafx.h"
#include "MathHelper.h"
#include "RenderSystem.h"
#include "LevelLoader.h"
#include "Registry.h"
#include "GameplaySystem.h"
#include "StepTimer.h"


class Core {
public:
	Core(int wndWidth, int wndHeight);
	
	void initialize();
	LRESULT run(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	void destroy();

	static const int mSwapChainBufferCount = 2;

	HWND hwnd;

	int getWndWidth() const;
	int getWndHeight() const;

private:

	void onResize();
	void onUpdate();
	void onRender();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

	void initD3D12Device();
	void createSwapChain();
	void createRTandDSViews();
	void loadPipeline();
	void loadAssets();
	void initImgui(float mainScale);
	void flushCommandQueue();

	void renderImgui();

	ComPtr<ID3D12Resource> getCurrentBackBuffer() const;

	D3D12_CPU_DESCRIPTOR_HANDLE getCurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilView();

	std::shared_ptr<Registry> registry;

	std::unique_ptr<LevelLoader> levelLoader;
	RenderSystem renderSystem;
	GameplaySystem gameplaySystem;

	int mCurrBackBuffer = 0;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	ComPtr<IDXGIFactory4> mFactory;
	ComPtr<ID3D12Device3> mDevice;
	ComPtr<ID3D12Fence> mFence;
	ComPtr<ID3D12CommandQueue> mCmdQueue;
	ComPtr<ID3D12CommandAllocator> mCmdAlloc;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;
	ComPtr<IDXGISwapChain> mSwapChain;
	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	ComPtr<ID3D12Resource> mSwapChainBuffer[mSwapChainBufferCount];
	ComPtr<ID3D12Resource>mDepthStencilBuffer;
	ComPtr<ID3D12PipelineState> mPipelineState;
	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize, mDsvDescriptorSize;
	UINT mCurrentFence;

	bool mUseMSAA;
	UINT mMSAAQualityLevel;

	float mTheta = XM_PIDIV2;
	float mPhi = XM_PIDIV2;
	float mRadius = 5.0f;
	POINT mLastMousePos;
	bool mMinimized, mMaximized;
	bool mUpdateView;
	
	int mWndWidth, mWndHeight;

	StepTimer timer;
};


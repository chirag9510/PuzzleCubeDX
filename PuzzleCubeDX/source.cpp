#include "Core.h"

std::unique_ptr<Core> gCore;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	return gCore->run(hwnd, msg, wparam, lparam);
}
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprevinstance, PSTR mCmdLine, int iShowCmd) {

	gCore = std::make_unique<Core>(680, 680);

	WNDCLASSEX wndclass = { 0 };
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.hInstance = hinstance;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.lpszClassName = L"WndClass";
	
	if (!RegisterClassEx(&wndclass)) {
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	RECT windowRect = { 0, 0, static_cast<LONG>(gCore->getWndWidth()), static_cast<LONG>(gCore->getWndHeight()) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create the window and store a handle to it.
	gCore->hwnd = CreateWindow(wndclass.lpszClassName, L"PuzzleCubeDX", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, 0, 0, hinstance, 0);
	if (!gCore->hwnd) {
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}


	gCore->initialize();

	ShowWindow(gCore->hwnd, SW_SHOW);

	MSG msg = {};
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}


	gCore->destroy();

	return 0;
}
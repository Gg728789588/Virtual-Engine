//***************************************************************************************
// d3dApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "d3dApp.h"
#include "Memory.h"
#include "Input.h"
#include <WindowsX.h>
#include "../IRendererBase.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	StackObject<D3DApp> d3dApp;
	IRendererBase* renderBase = GetRenderer();

	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	try
	{
#endif
		d3dApp.New(hInstance);
		if (!d3dApp->Initialize(renderBase))
			return 0;
		int value = d3dApp->Run();
		if (value == -1)
		{
			return 0;
		}
		d3dApp.Delete();
#if defined(DEBUG) | defined(_DEBUG)
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
#endif
}


LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp* D3DApp::mApp = nullptr;
D3DApp* D3DApp::GetApp()
{
	return mApp;
}
D3DApp::D3DApp(HINSTANCE hInstance)
{
	memset(&dataPack.settings, 0, sizeof(dataPack.settings));
	dataPack.settings.mClientWidth = 2560;
	dataPack.settings.mClientHeight = 1440;
	dataPack.settings.backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	dataPack.settings.mhAppInst = hInstance;

	if (mApp) delete mApp;
	mApp = this;
}
#undef RELEASE_PTR

D3DApp::~D3DApp()
{
	renderBaseFuncs->Dispose(dataPack);
	delete renderBaseFuncs;
	mApp = nullptr;
	dataPack.adapter->Release();
	dataPack.mdxgiFactory = nullptr;
	dataPack.md3dDevice = nullptr;
#undef RELEASE_PTR
}

int D3DApp::Run()
{
	MSG msg = { 0 };

	timer.Reset();
	while (msg.message != WM_QUIT)
	{

		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff.
		else
		{
			timer.Tick();
			if (!dataPack.settings.mAppPaused)
			{
				CalculateFrameStats();
				if (!renderBaseFuncs->Draw(dataPack, timer))
				{
					return -1;
				}
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}
bool windowsInitialized = false;
bool D3DApp::Initialize(IRendererBase* renderBase)
{
	renderBaseFuncs = renderBase;

	if (!windowsInitialized)
	{
		if (!InitMainWindow())
			return false;
		windowsInitialized = true;
	}
	if (!InitDirect3D())
		return false;
	if (!renderBase->Initialize(dataPack))
		return false;
	OnResize();
	return true;
}



void D3DApp::OnResize()
{
	assert(dataPack.md3dDevice);
	renderBaseFuncs->OnResize(dataPack);
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			OnPressMinimizeKey(true);
			dataPack.settings.mAppPaused = true;
			timer.Stop();
		}
		else
		{
			OnPressMinimizeKey(false);
			dataPack.settings.mAppPaused = false;
			timer.Start();
		}
		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		dataPack.settings.mClientWidth = LOWORD(lParam);
		dataPack.settings.mClientHeight = HIWORD(lParam);
		if (dataPack.md3dDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				dataPack.settings.mAppPaused = true;
				dataPack.settings.mMinimized = true;
				dataPack.settings.mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				dataPack.settings.mAppPaused = false;
				dataPack.settings.mMinimized = false;
				dataPack.settings.mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{

				// Restoring from minimized state?
				if (dataPack.settings.mMinimized)
				{
					dataPack.settings.mAppPaused = false;
					dataPack.settings.mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if (dataPack.settings.mMaximized)
				{
					dataPack.settings.mAppPaused = false;
					dataPack.settings.mMaximized = false;
					OnResize();
				}
				else if (dataPack.settings.mResizing)
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or dataPack.mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		dataPack.settings.mAppPaused = true;
		dataPack.settings.mResizing = true;
		timer.Stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		dataPack.settings.mAppPaused = false;
		dataPack.settings.mResizing = false;
		timer.Start();
		OnResize();
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
		Input::Input::inputData[!Input::inputDataSwitcher].lMouseDown = true;
		return 0;
	case WM_MBUTTONDOWN:
		Input::Input::inputData[!Input::inputDataSwitcher].mMouseDown = true;
		return 0;
	case WM_RBUTTONDOWN:
		//OnRMouseDown(wParam);
		Input::Input::inputData[!Input::inputDataSwitcher].rMouseDown = true;
		return 0;
	case WM_LBUTTONUP:
		Input::Input::inputData[!Input::inputDataSwitcher].lMouseUp = true;
		//OnLMouseUp(wParam);
		return 0;
	case WM_MBUTTONUP:
		Input::Input::inputData[!Input::inputDataSwitcher].mMouseUp = true;
		//OnMMouseUp(wParam);
		return 0;
	case WM_RBUTTONUP:
		Input::Input::inputData[!Input::inputDataSwitcher].mMouseUp = true;
		//OnRMouseUp(wParam);
		return 0;
	case WM_MOUSEMOVE:
	{
		Input::inputData[!Input::inputDataSwitcher].mouseState = wParam;
		int2 mouseMove = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		Input::OnMoveMouse({ mouseMove.x, mouseMove.y });
	}
	return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		wParam = Min<uint>(wParam, 105);
		Input::Input::inputData[!Input::inputDataSwitcher].keyUpArray[wParam] = true;
		return 0;
	case WM_KEYDOWN:
		wParam = Min<uint>(wParam, 105);
		Input::Input::inputData[!Input::inputDataSwitcher].keyDownArray[wParam] = true;
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
HWND      mhMainWnd = nullptr; // main window handle
bool D3DApp::InitMainWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = dataPack.settings.mhAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, dataPack.settings.mClientWidth, dataPack.settings.mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"MainWnd", L"Virtual Engine",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, dataPack.settings.mhAppInst, 0);
	dataPack.mhMainWnd = mhMainWnd;
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

bool D3DApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dataPack.mdxgiFactory)));
	/*
		// Try to create hardware device.
		HRESULT hardwareResult = D3D12CreateDevice(
			nullptr,             // default adapter
			D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(&dataPack.md3dDevice));

		// Fallback to WARP device.
		if (FAILED(hardwareResult))
		{
			ComPtr<IDXGIAdapter> pWarpAdapter;
			ThrowIfFailed(dataPack.mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

			ThrowIfFailed(D3D12CreateDevice(
				pWarpAdapter.Get(),
				D3D_FEATURE_LEVEL_12_0,
				IID_PPV_ARGS(&dataPack.md3dDevice)));
		}*/
	int adapterIndex = 0; // we'll start looking for directx 12  compatible graphics devices starting at index 0

	bool adapterFound = false; // set this to true when a good one was found
	while (dataPack.mdxgiFactory->EnumAdapters1(adapterIndex, &dataPack.adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		dataPack.adapter->GetDesc1(&desc);

		if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
		{
			HRESULT hr = D3D12CreateDevice(dataPack.adapter, D3D_FEATURE_LEVEL_12_0,
				IID_PPV_ARGS(&dataPack.md3dDevice));
			if (SUCCEEDED(hr))
			{
				adapterFound = true;
				break;
			}
		}
		dataPack.adapter->Release();
		adapterIndex++;
	}

	dataPack.settings.mRtvDescriptorSize = dataPack.md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dataPack.settings.mDsvDescriptorSize = dataPack.md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	dataPack.settings.mCbvSrvUavDescriptorSize = dataPack.md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.


#ifdef _DEBUG
	LogAdapters();
#endif

	return true;
}




void D3DApp::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((timer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		vengine::wstring fpsStr = vengine::to_wstring(fps);
		vengine::wstring mspfStr = vengine::to_wstring(mspf);

		vengine::wstring windowText = L"Virtual Engine";
		windowText +=
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr + L" ";
		if (dataPack.settings.windowInfo) windowText += dataPack.settings.windowInfo;

		SetWindowText(mhMainWnd, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void D3DApp::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	vengine::vector<IDXGIAdapter*> adapterList;
	while (dataPack.mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		vengine::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);

		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		vengine::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, dataPack.settings.backBufferFormat);

		ReleaseCom(output);

		++i;
	}
}

void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	// Call with nullptr to get list count.
	output->GetDisplayModeList(format, flags, &count, nullptr);

	vengine::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		vengine::wstring text =
			L"Width = " + vengine::to_wstring(x.Width) + L" " +
			L"Height = " + vengine::to_wstring(x.Height) + L" " +
			L"Refresh = " + vengine::to_wstring(n) + L"/" + vengine::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}
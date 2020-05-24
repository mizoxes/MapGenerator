// this program generates a heightmap using Perlin noise algorithm
// and saves it as a ppm file and then loads and applies it to a 2D grid

// Windows stuff
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

using namespace DirectX;

// C++ standard library stuff
#include <iostream>
#include <vector>

// my stuff
#include "PerlinNoise.h"
#include "ppm.h"
#include "Terrain.h"

// Structures
struct ConstantBuffer
{
	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProjection;
};

// Global variables
HINSTANCE               g_hInst = nullptr;
HWND                    g_hWnd = nullptr;
ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11DeviceContext*    g_pImmediateContext = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11Texture2D*        g_pDepthStencilBuffer = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
ID3D11VertexShader*     g_pVertexShader = nullptr;
ID3D11PixelShader*      g_pPixelShader = nullptr;
ID3D11InputLayout*      g_pVertexLayout = nullptr;
ID3D11Buffer*           g_pVertexBuffer = nullptr;
ID3D11Buffer*           g_pIndexBuffer = nullptr;
ID3D11Buffer*           g_pConstantBuffer = nullptr;
ID3D11RasterizerState*  g_pRSWireframe = nullptr;
UINT                    g_IndexCount = 0;
XMMATRIX				g_World;
XMMATRIX				g_View;
XMMATRIX				g_Projection;
float                   g_fRadius = 500.0f;
float                   g_fPhi = 0.35f * XM_PI;
float                   g_fTheta = 1.3f * XM_PI;

// Function prototypes
bool InitWindow(HINSTANCE hInstance);
bool InitDirect3D();
void CleanupDirect3D();
LRESULT CALLBACK MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Update(float deltaTime);
void Render();


// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	constexpr int img_width = 256;
	constexpr int img_height = 256;
	ppm image(img_width, img_height);
	PerlinNoise pn(237);
	int kk = 0;
	for (int i = 0; i < img_height; ++i)
	{
		for (int j = 0; j < img_width; ++j)
		{
			double x = j / (double)img_width;
			double y = i / (double)img_height;

			double n = pn.noise(10 * x, 10 * y, 0.8);

			image.r[kk] = floor(n * 255);
			image.g[kk] = floor(n * 255);
			image.b[kk] = floor(n * 255);
			kk++;
		}
	}
	image.write("perlin.ppm");

	if (!InitWindow(hInstance))
		return 0;

	if (!InitDirect3D())
	{
		CleanupDirect3D();
		return 0;
	}

	__int64 current, previous, frequency;
	QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
	previous = 0;

	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			QueryPerformanceCounter((LARGE_INTEGER*)&current);
			float deltaTime = (float)(current - previous) / frequency;
			previous = current;

			Update(deltaTime);
			Render();
		}
	}

	CleanupDirect3D();

	return (int)msg.wParam;
}

bool InitWindow(HINSTANCE hInstance)
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MessageHandler;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = L"MAPGEN";
	wc.lpszMenuName = nullptr;
	if (!RegisterClass(&wc))
		return false;

	RECT rc = { 0, 0, 800, 600 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, false);
	g_hWnd = CreateWindow(L"MAPGEN", L"Map Generator (use Arrow keys to control the camera)",
		WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top,
		nullptr, nullptr, hInstance, nullptr);
	if (!g_hWnd)
		return false;

	ShowWindow(g_hWnd, SW_SHOWNORMAL);

	return true;
}

bool InitDirect3D()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
		featureLevels, numFeatureLevels, D3D11_SDK_VERSION,
		&g_pd3dDevice, nullptr, &g_pImmediateContext);
	if (FAILED(hr))
		return false;

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferCount = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	IDXGIDevice* dxgiDevice = nullptr;
	hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	if (FAILED(hr))
		return false;

	IDXGIAdapter* dxgiAdapter = nullptr;
	hr = dxgiDevice->GetAdapter(&dxgiAdapter);
	if (FAILED(hr))
		return false;

	IDXGIFactory* dxgiFactory = nullptr;
	hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
	if (FAILED(hr))
		return false;

	hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);
	if (FAILED(hr))
		return false;

	dxgiDevice->Release();
	dxgiAdapter->Release();
	dxgiFactory->Release();

	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
	if (FAILED(hr))
		return false;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return false;

	D3D11_TEXTURE2D_DESC dbd;
	dbd.Width = width;
	dbd.Height = height;
	dbd.MipLevels = 1;
	dbd.ArraySize = 1;
	dbd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dbd.SampleDesc.Count = 1;
	dbd.SampleDesc.Quality = 0;
	dbd.Usage = D3D11_USAGE_DEFAULT;
	dbd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dbd.CPUAccessFlags = 0;
	dbd.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&dbd, nullptr, &g_pDepthStencilBuffer);
	if (FAILED(hr))
		return false;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
	ZeroMemory(&dsvd, sizeof(dsvd));
	dsvd.Format = dbd.Format;
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvd.Flags = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &dsvd, &g_pDepthStencilView);
	if (FAILED(hr))
		return false;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = (float)width;
	vp.Height = (float)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	
	g_pImmediateContext->RSSetViewports(1, &vp);

	ID3D10Blob* pVSBlob = nullptr;
	hr = D3DReadFileToBlob(L"../Debug/VertexShader.cso", &pVSBlob);
	if (FAILED(hr))
		return false;

	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = g_pd3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return false;

	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	ID3D10Blob* pPSBlob = nullptr;
	hr = D3DReadFileToBlob(L"../Debug/PixelShader.cso", &pPSBlob);
	if (FAILED(hr))
		return false;

	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
	{
		return false;
	}

	Terrain terrain("perlin.ppm");

	g_IndexCount = terrain.indices.size();

	D3D11_BUFFER_DESC bd;
	bd.ByteWidth = sizeof(Vertex) * terrain.vertices.size();
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = &terrain.vertices[0];
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, &initData, &g_pVertexBuffer);
	if (FAILED(hr))
		return false;

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	bd.ByteWidth = sizeof(UINT) * terrain.indices.size();
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;
	initData.pSysMem = &terrain.indices[0];
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, &initData, &g_pIndexBuffer);
	if (FAILED(hr))
		return false;

	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pConstantBuffer);
	if (FAILED(hr))
		return false;

	D3D11_RASTERIZER_DESC rd;
	ZeroMemory(&rd, sizeof(rd));
	rd.CullMode = D3D11_CULL_BACK;
	rd.FillMode = D3D11_FILL_WIREFRAME;
	rd.DepthClipEnable = true;

	hr = g_pd3dDevice->CreateRasterizerState(&rd, &g_pRSWireframe);
	if (FAILED(hr))
		return false;

	//g_pImmediateContext->RSSetState(g_pRSWireframe);

	g_World = XMMatrixIdentity();
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (float)height, 0.1f, 5000.0f);

	return true;
}

void CleanupDirect3D()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	if (g_pRSWireframe) g_pRSWireframe->Release();
	if (g_pConstantBuffer) g_pConstantBuffer->Release();
	if (g_pIndexBuffer) g_pIndexBuffer->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pDepthStencilBuffer) g_pDepthStencilBuffer->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}

LRESULT CALLBACK MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;

	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 1;

	case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		return 1;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void Update(float deltaTime)
{
	if (GetAsyncKeyState(VK_LEFT))
		g_fTheta -= deltaTime * 1.5f;
	else if (GetAsyncKeyState(VK_RIGHT))
		g_fTheta += deltaTime * 1.5f;
	if (GetAsyncKeyState(VK_UP))
		g_fRadius -= deltaTime * 200.0f;
	if (GetAsyncKeyState(VK_DOWN))
		g_fRadius += deltaTime * 200.0f;

	// Convert spherical to cartesian
	float x = g_fRadius * sinf(g_fPhi) * cosf(g_fTheta);
	float y = g_fRadius * cosf(g_fPhi);
	float z = g_fRadius * sinf(g_fPhi) * sinf(g_fTheta);

	XMVECTOR Eye = XMVectorSet(x, y, z, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	g_View = XMMatrixLookAtLH(Eye, At, Up);
}

void Render()
{
	float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	ConstantBuffer cb;
	XMStoreFloat4x4(&cb.mWorld, XMMatrixTranspose(g_World));
	XMStoreFloat4x4(&cb.mView, XMMatrixTranspose(g_View));
	XMStoreFloat4x4(&cb.mProjection, XMMatrixTranspose(g_Projection));
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
	g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);

	g_pImmediateContext->DrawIndexed(g_IndexCount, 0, 0);

	g_pSwapChain->Present(1, 0);
}
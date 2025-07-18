// ClearScreen.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>
#include <iostream>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

ID3D12Device* device = nullptr;
IDXGIFactory4* factory = nullptr;
ID3D12CommandQueue* commandQueue = nullptr;
IDXGISwapChain3* swapChain = nullptr;
ID3D12DescriptorHeap* rtvHeap = nullptr;
ID3D12Resource* renderTargets[2];
UINT rtvDescriptorSize = 0;
ID3D12CommandAllocator* commandAlloc = nullptr;
ID3D12GraphicsCommandList* commandList = nullptr;

uint32_t width = 1280;
uint32_t height = 800;

bool InitializeDirectX(HWND hWnd)
{

    D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));


    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


    IDXGISwapChain1* tempSwapChain = nullptr;

    factory->CreateSwapChainForHwnd(commandQueue, hWnd, &swapChainDesc, nullptr, nullptr, &tempSwapChain);
    tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain));
    factory->Release();


    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 2;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

    for (int i = 0; i < 2; ++i)
    {
        ID3D12Resource* backBuffer = nullptr;
        swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += i * rtvDescriptorSize;
        device->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);

        renderTargets[i] = backBuffer;
    }

    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAlloc));
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAlloc, nullptr, IID_PPV_ARGS(&commandList));


    commandList->Close();

    return true;
}


void Render()
{
    commandAlloc->Reset();
    commandList->Reset(commandAlloc, nullptr);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += (swapChain->GetCurrentBackBufferIndex()) * rtvDescriptorSize;
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    commandList->Close();

    ID3D12CommandList* ppCommandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(1, ppCommandLists);

    // Presentar la imagen.
    swapChain->Present(1, 0);
}

int main()
{

    WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"DX12WindowClass", nullptr };
    RegisterClassEx(&wcex);
    HWND hWnd = CreateWindow(L"DX12WindowClass", L"DirectX 12 Minimal Example", WS_OVERLAPPEDWINDOW, 100, 100, width, height, nullptr, nullptr, wcex.hInstance, nullptr);
    ShowWindow(hWnd, SW_SHOW);

    InitializeDirectX(hWnd);


    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Render();
        }
    }

    for (int i = 0; i < 2; ++i)
        if (renderTargets[i]) renderTargets[i]->Release();

    if (rtvHeap)
        rtvHeap->Release();

    if (swapChain)
        swapChain->Release();

    if (commandQueue)
        commandQueue->Release();

    if (device)
        device->Release();

    if (commandAlloc)
        commandAlloc->Release();

    if (commandList)
        commandList->Release();

    return 0;
}

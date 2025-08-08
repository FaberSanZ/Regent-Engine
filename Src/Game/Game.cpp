// Game.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")


#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#include "texture.h" // sprite sheet


#define TITLE L"Sprite renderer"


int main()
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, TITLE, nullptr };
    RegisterClassEx(&wcex);

    HWND window = CreateWindow(TITLE, TITLE, WS_OVERLAPPEDWINDOW, 100, 100, 1200, 820, nullptr, nullptr, wcex.hInstance, nullptr);

    ShowWindow(window, SW_SHOW);




    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    ID3D11Device* baseDevice;
    ID3D11DeviceContext* baseDeviceContext;

    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &baseDevice, nullptr, &baseDeviceContext);


    ID3D11Device1* device;

    baseDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&device));

    ID3D11DeviceContext1* deviceContext;

    baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&deviceContext));


    IDXGIDevice1* dxgiDevice;

    device->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void**>(&dxgiDevice));

    IDXGIAdapter* dxgiAdapter;

    dxgiDevice->GetAdapter(&dxgiAdapter);

    IDXGIFactory2* dxgiFactory;

    dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory));


    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = 0; // use window width
    swapChainDesc.Height = 0; // use window height
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    IDXGISwapChain1* swapChain;

    dxgiFactory->CreateSwapChainForHwnd(device, window, &swapChainDesc, nullptr, nullptr, &swapChain);

    swapChain->GetDesc1(&swapChainDesc); // get actual window width & height


    ID3D11Texture2D* framebufferTexture;

    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&framebufferTexture));

    ID3D11RenderTargetView* framebufferRTV;

    device->CreateRenderTargetView(framebufferTexture, nullptr, &framebufferRTV);


    float constantData[4] = { 2.0f / swapChainDesc.Width, -2.0f / swapChainDesc.Height, 1.0f / ATLAS_WIDTH, 1.0f / ATLAS_HEIGHT }; // one-time calc here to make it simpler for the shader later (float2 rn_screensize, r_atlassize)

    D3D11_BUFFER_DESC constantBufferDesc = {};
    constantBufferDesc.ByteWidth = sizeof(constantData) + 0xf & 0xfffffff0; // ensure constant buffer size is multiple of 16 bytes
    constantBufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // constant buffer doesn't need updating in this example
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA constantSRD = { constantData };

    ID3D11Buffer* constantBuffer;

    device->CreateBuffer(&constantBufferDesc, &constantSRD, &constantBuffer);


    D3D11_TEXTURE2D_DESC atlasTextureDesc = {};
    atlasTextureDesc.Width = ATLAS_WIDTH;
    atlasTextureDesc.Height = ATLAS_HEIGHT;
    atlasTextureDesc.MipLevels = 1;
    atlasTextureDesc.ArraySize = 1;
    atlasTextureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    atlasTextureDesc.SampleDesc.Count = 1;
    atlasTextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    atlasTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA atlasSRD = {};
    atlasSRD.pSysMem = atlasData;
    atlasSRD.SysMemPitch = ATLAS_WIDTH * sizeof(UINT);

    ID3D11Texture2D* atlasTexture;

    device->CreateTexture2D(&atlasTextureDesc, &atlasSRD, &atlasTexture);

    ID3D11ShaderResourceView* atlasSRV;

    device->CreateShaderResourceView(atlasTexture, nullptr, &atlasSRV);


#define MAX_SPRITES 4096

    struct int2 { int x, y; };
    struct Sprite { int2 screenPos, size, atlasPos; float scale; };

    D3D11_BUFFER_DESC spriteBufferDesc = {};
    spriteBufferDesc.ByteWidth = MAX_SPRITES * sizeof(Sprite);
    spriteBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    spriteBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    spriteBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    spriteBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    spriteBufferDesc.StructureByteStride = sizeof(Sprite);

    ID3D11Buffer* spriteBuffer;

    device->CreateBuffer(&spriteBufferDesc, nullptr, &spriteBuffer);

    D3D11_SHADER_RESOURCE_VIEW_DESC spriteSRVDesc = {};
    spriteSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    spriteSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    spriteSRVDesc.Buffer.NumElements = MAX_SPRITES;

    ID3D11ShaderResourceView* spriteSRV;

    device->CreateShaderResourceView(spriteBuffer, &spriteSRVDesc, &spriteSRV);


    D3D11_SAMPLER_DESC pointsamplerDesc = {};
    pointsamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    pointsamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    pointsamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    pointsamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    pointsamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

    ID3D11SamplerState* pointSampler;

    device->CreateSamplerState(&pointsamplerDesc, &pointSampler);


    ID3DBlob* vsBlob;

    D3DCompileFromFile(L"../../Assets/Shaders/PixelShader.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &vsBlob, nullptr);

    ID3D11VertexShader* vertexShader;

    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);


    ID3DBlob* psBlob;

    D3DCompileFromFile(L"../../Assets/Shaders/PixelShader.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &psBlob, nullptr);

    ID3D11PixelShader* pixelShader;

    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);


    FLOAT clearColor[4] = { 0.0f, 0.2f, 0.4f, 1 };

    D3D11_VIEWPORT framebufferVP = { 0, 0, static_cast<float>(swapChainDesc.Width), static_cast<float>(swapChainDesc.Height), 0, 1 };


    Sprite* spriteBatch = reinterpret_cast<Sprite*>(HeapAlloc(GetProcessHeap(), 0, MAX_SPRITES * sizeof(Sprite)));

    float scale = 0.0f;
	int spriteCount = 24 * 24; // number of sprites to render in this example

    while (true)
    {
        MSG msg;

        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_KEYDOWN) return 0;
            DispatchMessageA(&msg);
        }


        static int tick = 0;
		scale += 0.01f;

        // Frame de animación
        int frame = tick++ / 5 % 10;
        //int2 cell = { frame % ATLAS_COLS, frame / ATLAS_COLS };
        int2 cell = { 1, 0 };


        // Tamaño del sprite
        int spriteW = (ATLAS_WIDTH / ATLAS_COLS);
        int spriteH = (ATLAS_HEIGHT / ATLAS_ROWS);

  //      for(int x = 0; x < 24; ++x)
  //      {
  //          for(int y = 0; y < 24; ++y)
  //          {
  //              int i = x + y * 24;
		//		int spriteIndex = (x + y * 24) % spriteCount; // wrap around if spriteCount is less than 576

  //              if (i >= MAX_SPRITES) 
  //                  break; // avoid overflow

  //              // Clear sprite
  //              spriteBatch[i] = {};
  //              // Set sprite position
  //              spriteBatch[i].screenPos.x = x * spriteW;
  //              spriteBatch[i].screenPos.y = y * spriteH;
  //              // Set sprite size
  //              spriteBatch[i].size.x = spriteW;
  //              spriteBatch[i].size.y = spriteH;
  //              // Set atlas position
  //              spriteBatch[i].atlasPos.x = cell.x * spriteW;
  //              spriteBatch[i].atlasPos.y = cell.y * spriteH;
  //              // Set scale
		//		spriteBatch[i].scale = scale;

		//	}
		//}

        // Sprite 1
        spriteBatch[0] = 
        {
            {300, 300},  // screenPos
            {spriteW, spriteH},
            {cell.x * spriteW, cell.y * spriteH},
            4
        };

        //// Sprite 2
        //spriteBatch[1] = 
        //{
        //    {300, 200},  // otra posición
        //    {spriteW, spriteH},
        //    {cell.x * spriteW, cell.y * spriteH},
        //    1
        //};


        //// Sprite 2
        //spriteBatch[2] = 
        //{
        //    {100, 400},  // otra posición
        //    {spriteW, spriteH},
        //    {cell.x * spriteW, cell.y * spriteH},
        //    1
        //};

		if (scale > 2.0f) scale = 0.0f; // reset scale after reaching a certain value

        ///////////////////////////////////////////////////////////////////////////////////////////

        D3D11_MAPPED_SUBRESOURCE spriteBufferMSR;

        deviceContext->Map(spriteBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &spriteBufferMSR);

        memcpy(spriteBufferMSR.pData, spriteBatch, spriteCount * sizeof(Sprite));

        deviceContext->Unmap(spriteBuffer, 0);

        ///////////////////////////////////////////////////////////////////////////////////////////

        deviceContext->OMSetRenderTargets(1, &framebufferRTV, nullptr);

        deviceContext->ClearRenderTargetView(framebufferRTV, clearColor);

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); // to render sprite quad using 4 vertices

        deviceContext->VSSetShader(vertexShader, nullptr, 0);
        deviceContext->VSSetShaderResources(0, 1, &spriteSRV);
        deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);

        deviceContext->RSSetViewports(1, &framebufferVP);

        deviceContext->PSSetShader(pixelShader, nullptr, 0);
        deviceContext->PSSetShaderResources(1, 1, &atlasSRV);
        deviceContext->PSSetSamplers(0, 1, &pointSampler);

        ///////////////////////////////////////////////////////////////////////////////////////////

        deviceContext->DrawInstanced(4, spriteCount, 0, 0);
        swapChain->Present(1, 0);
    }
}

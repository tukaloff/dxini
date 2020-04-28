#pragma once
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN    // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include <string>
#include "resource.h"

#include <dxgi.h>
#include "framework.h"

#include <d3dcompiler.h>

// this will only call release if an object exists (prevents exceptions calling release on non existant objects)
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

using namespace DirectX; // we will be using the directxmath library

HWND hWnd;

// name of the window (not the title)
LPCTSTR WindowName = L"RenderWindow";

// title of the window
LPCTSTR WindowTitle = L"Render Window";

// width and height of the window
int Width = 800;
int Height = 600;

// is window full screen?
bool FullScreen = false;

// we will exit the program when this becomes false
bool Running = true;

// create a window
bool InitializeWindow(HINSTANCE hInstance,
    int ShowWnd,
    bool fullscreen);

// main application loop
void mainloop();

// callback function for windows messages
LRESULT CALLBACK WndProc(HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

// DirectX12 stuff
const int frameBufferCount = 3;
ID3D12Device* device; //d3d device
IDXGISwapChain3* swapChain; //switch between render targets
ID3D12CommandQueue* commandQueue; //container for command list
ID3D12DescriptorHeap* rtvDescriptorHeap; //a descriptor heap to hold resources like render targets
ID3D12Resource* renderTargets[frameBufferCount]; //number of render targets equal to frame buffer count
ID3D12CommandAllocator* commandAllocator[frameBufferCount]; //we want enough allocator for each buffer * number of threads (we have only one thread)
ID3D12GraphicsCommandList* commandList; //a command list we can record commands into, then execute then to render the frame
ID3D12Fence* fence[frameBufferCount]; //an object that is locked while our command list is being executed by the gpu.
                                     //We need as many as we have allocators (more if we want to know when the gpu is finished with an asset)
HANDLE fenceEvent; //a handle to an event when our fence is unlocked by gpu
UINT64 fenceValue[frameBufferCount]; //this value is incremented each frame. each fence will have its own value
int frameIndex; //current rtv we are on
int rtvDescriptorSize;  //size for the rtv descriptor on the device (all front and back buffers will be the same size)

// function declarations
bool InitD3D(); //initalizes direct3d 12
void Update(); //update the game logic
void UpdatePipeline(); //update the direct3d pipeline (update command list)
void Render(); //execute the command list
void Cleanup(); //release com object and clean up memory
void WaitForPreviousFrame(); //wait until gpu is finished with command list
// ===============


#define MAX_LOADSTRING 100

// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
//INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);



ID3D12PipelineState* pipelineStateObject; // pso containing a pipeline state
ID3D12RootSignature* rootSignature; // root signature defines data shaders will access
D3D12_VIEWPORT viewport; // area that output from rasterizer will be stretched to
D3D12_RECT scissorRect; // the area to draw in. pixels outside that area will not be drawn onto
ID3D12Resource* vertexBuffer; // a default buffer in gpu memory that we will load vertex data for our triangle into
D3D12_VERTEX_BUFFER_VIEW vertexBufferView; // a structure containing a pointer to the vertex data in gpu memory
                                           // the total size of the buffer and the size of each element (vertex)

ID3D12Resource* indexBuffer; // a default buffer in gpu memory that we will load index data forour triangle into
D3D12_INDEX_BUFFER_VIEW indexBufferView; // a structure holding information about the index buffer

ID3D12Resource* depthStencilBuffer; // this is a memory for our depth buffer
ID3D12DescriptorHeap* dsDescriptorHeap; // this is a heap for our depth/stencil buffer descriptor

struct ConstantBuffer {
    XMFLOAT4 colorMultiplier;
};

ID3D12DescriptorHeap* mainDescriptorHeap[frameBufferCount]; // this heap will store the descriptor to our constant buffer
ID3D12Resource* constantBufferUploadHeap[frameBufferCount]; // this is the memory on the gpu where our constant buffer will be placed

ConstantBuffer cbColorMultiplierData; // whit is constant buffer data we will send to the gpu (which will be placed in the resource we created above)

UINT8* cbColorMultiplierGPUAddress[frameBufferCount]; // // this is a pointer to the memory location we get when we map our constant buffer

struct ConstantBufferPerObject
{
    XMFLOAT4X4 wvpMat;
};

int ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBufferPerObject) + 255) & ~255;
ConstantBufferPerObject cbPerObject;
ID3D12Resource* constantBufferUploadHeaps[frameBufferCount];
UINT8* cbvGPUAddress[frameBufferCount];

XMFLOAT4X4 cameraProjMat;
XMFLOAT4X4 cameraViewMat;
XMFLOAT4 cameraPosition;
XMFLOAT4 cameraTarget;
XMFLOAT4 cameraUp;
XMFLOAT4X4 cube1WorldMat;
XMFLOAT4X4 cube1RotMat;
XMFLOAT4 cube1Position;
XMFLOAT4X4 cube2WorldMat;
XMFLOAT4X4 cube2RotMat;
XMFLOAT4 cube2PositionOffset;

int numCubeIndices;

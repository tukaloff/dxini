﻿// dxini.cpp : Определяет точку входа для приложения.
//

#include "dxini.h"

LPCWSTR errorMsg;

struct Vertex {
	Vertex(float x, float y, float z, float u, float v) : pos(x, y, z), texCoord(u, v) {}
	XMFLOAT3 pos;
	XMFLOAT2 texCoord;
};

struct TextVertex {
	TextVertex(float r, float g, float b, float a, float u, float v, float tw, float th, float x, float y, float w, float h) : color(r, g, b, a), texCoord(u, v, tw, th), pos(x, y, w, h) {}
	XMFLOAT4 pos;
	XMFLOAT4 texCoord;
	XMFLOAT4 color;
};

int WINAPI WinMain(HINSTANCE hInstance,    //Main windows function
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
	Properties* props = new Properties();
	props->load();
	
	string fbxPropPath = "path.model";
	string fbxPath = props->get(&fbxPropPath);// "model/Flat2.fbx";
	string fbxOutPropPath = "path.xmlOut";
	string fbxOutPath = props->get(&fbxOutPropPath);// "model/Flat2.fbx";

	FbxUtils* fbx = new FbxUtils(fbxPath.c_str());
	fbx->test(&fbxOutPath);

	// create the window
	if (!InitializeWindow(hInstance, nShowCmd, FullScreen))
	{
		MessageBox(0, L"Window Initialization - Failed",
			L"Error", MB_OK);
		return 1;
	}

	// initialize direct3d
	if (!InitD3D())
	{
		MessageBox(0, errorMsg,
			L"Error", MB_OK);
		Cleanup();
		return 1;
	}

	// start the main loop
	mainloop();

	// we want to wait for the gpu to finish executing the command list before we start releasing everything
	WaitForPreviousFrame();

	// close the fence event
	CloseHandle(fenceEvent);

	// clean up everything
	Cleanup();

	return 0;
}

// create and show the window
bool InitializeWindow(HINSTANCE hInstance,
	int ShowWnd,
	bool fullscreen)
{
	if (fullscreen)
	{
		HMONITOR hmon = MonitorFromWindow(hWnd,
			MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(hmon, &mi);

		Width = mi.rcMonitor.right - mi.rcMonitor.left;
		Height = mi.rcMonitor.bottom - mi.rcMonitor.top;
	}

	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WindowName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Error registering class",
			L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	hWnd = CreateWindowEx(NULL,
		WindowName,
		WindowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		Width, Height,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (!hWnd)
	{
		MessageBox(NULL, L"Error creating window",
			L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	if (fullscreen)
	{
		SetWindowLong(hWnd, GWL_STYLE, 0);
	}

	ShowWindow(hWnd, ShowWnd);
	UpdateWindow(hWnd);

	return true;
}

void mainloop() {
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	while (Running)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			// run game code
			double delta = timer.GetFrameDelta();
			Update(delta); // update the game logic
			Render(); // execute the command queue (rendering the scene is the result of the gpu executing the command lists)
		}
	}
}

bool InitD3D()
{
	HRESULT hr;

	// -- create the device -- //

	IDXGIFactory4* dxgiFactory;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateDXGIFactory1";
		return false;
	}

	IDXGIAdapter1* adapter; // adapters are the graphic card (this included the embedded graphics on the motherboard)
	int adapterIndex = 0; // we'll start looking for directx 12 compatible graphics devices starting at index 0
	bool adapterFound = false;

	// find first hardware gpu that support directx 12
	while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// we don't want software device
			adapterIndex++;
			continue;
		}

		// we want a device that is compatible with direct3d 12 (feature level 11 or higher)
		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr))
		{
			adapterFound = true;
			break;
		}

		adapterIndex++;
	}

	if (!adapterFound)
	{
		errorMsg = L"Failed create D3D12CreateDevice";
		Running = false;
		return false;
	}

	// Create the device
	hr = D3D12CreateDevice(
		adapter,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&device)
	);
	if (FAILED(hr))
	{
		errorMsg = L"Failed create D3D12CreateDevice";
		Running = false;
		return false;
	}

	// -- create command queue -- //
	D3D12_COMMAND_QUEUE_DESC cqDesc = {}; // we will be using all the default values
	cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // direct means the gpu can directly execute this command queue

	hr = device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue)); // create the command queue
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateCommandQueue";
		Running = false;
		return false;
	}

	// -- create the swap chain (double/tripple buffering) -- //
	DXGI_MODE_DESC backBufferDesc = {}; // this is to describe our display mode
	backBufferDesc.Width = Width; // buffer width
	backBufferDesc.Height = Height; // buffer height
	backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the buffer (rgba 32 bits, 8 bits for each channel)

	// describe our multi-sampling. We are not multi-sampling, so we set the count to 1 (we need at least 1 sample)
	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;

	// describe and create the swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = frameBufferCount; // number of buffer we have
	swapChainDesc.BufferDesc = backBufferDesc; // our back buffer description
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // this says the pipeline will render to this swap chain
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // dxgi will discard the buffer (data) after we call present
	swapChainDesc.OutputWindow = hWnd; // handle to our window
	swapChainDesc.SampleDesc = sampleDesc; // our multi-sampling description
	swapChainDesc.Windowed = !FullScreen; // set to true, then if in fullscreen must call SetFullScreenState with true for full screen to get uncapped fps

	IDXGISwapChain* tempSwapChain;

	dxgiFactory->CreateSwapChain(
		commandQueue, // the queue will be flushed once the swap chain is created
		&swapChainDesc, // give it the swap chain description we created above
		&tempSwapChain // store the created swap chain in a temp IDXGISwapChain interface
	);

	swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);

	frameIndex = swapChain->GetCurrentBackBufferIndex();

	// -- create the back buffers (render target views) descriptorheap -- //

	// describe an rtv descriptor heap and create
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = frameBufferCount; // number of descriptors for this heap.
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // this heap is a render target view heap

	// this heap will not be directly referenced by the shaders (not shader visible),
	// as this will store the output from the pipeline
	// otherwise we will set the heap's flag to D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateDescriptorHeap";
		Running = false;
		return false;
	}

	// get the size of a descriptor in this heap )this is a rtv heap, so only rtv descriptors should be stored in it.
	// descriptor sizes may vary from device to device, which is why there is no set size and we must ask the device
	// to give us the size. we will usethis size to increment a descriptor handle offset
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// get a handle to the first descriptor in the descriptor heap. a handle is basycally a pointer,
	// but we cannot literally use it like a c++ pointer
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// create a rtv for each buffer (double buffering is two buffers, tripple buffering is 3)
	for (int i = 0; i < frameBufferCount; i++)
	{
		// first we get the n'th buffer in the swap chain and store it in the n'th
		// position of the ID3D12Resource array
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
		if (FAILED(hr))
		{
			errorMsg = L"Failed create GetBuffer";
			Running = false;
			return false;
		}

		// "create" a render target view which binds the swap chain buffer (ID3D12Resource[n]) to the rtv handle
		device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);

		// we increment the rtv handle by the rtv descriptor size we got above
		rtvHandle.Offset(1, rtvDescriptorSize);
	}

	// -- create the command allocators -- //
	for (int i = 0; i < frameBufferCount; i++)
	{
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i]));
		if (FAILED(hr))
		{
			errorMsg = L"Failed create CreateCommandAllocator";
			Running = false;
			return false;
		}
	}

	// -- create a command list -- //

	// create the command list with the first allocator
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[frameIndex], NULL, IID_PPV_ARGS(&commandList));
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateCommandList";
		return false;
	}

	// command list are created inthe recording state. our main loop will set it up for recording again so close it now
	//commandList->Close();

	// -- create a fence & fence event -- //

	// create the fences
	for (int i = 0; i < frameBufferCount; i++)
	{
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence[i]));
		if (FAILED(hr))
		{
			errorMsg = L"Failed create CreateFence";
			Running = false;
			return false;
		}
		fenceValue[i] = 0;
	}

	// create a handle to a fence event
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr)
	{
		errorMsg = L"Failed create CreateEvent";
		Running = false;
		return false;
	}

	// create root signature

	// create a root descriptor, which explains where to find the data for this root parameter
	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;
	rootCBVDescriptor.ShaderRegister = 0;

	// create a descriptor range (descriptor table)
	D3D12_DESCRIPTOR_RANGE descriptorTableRanges[1];
	descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorTableRanges[0].NumDescriptors = 1;
	descriptorTableRanges[0].BaseShaderRegister = 0;
	descriptorTableRanges[0].RegisterSpace = 0;
	descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// create a descriptor table
	D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
	descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges);
	descriptorTable.pDescriptorRanges = &descriptorTableRanges[0];

	// create a root parameter and fill it out
	D3D12_ROOT_PARAMETER rootParameters[2]; // only one parameter right now
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // this is a constant buffer view root descriptor
	rootParameters[0].Descriptor = rootCBVDescriptor; // this is the root descriptor for this root parameter
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable = descriptorTable;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// create static sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), // we have 1 root parameter
		rootParameters, // a pointer to the beginning of our root parameters array
		1,
		&sampler,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	ID3DBlob* errorBuff; // a buffer holding the error data if any
	ID3DBlob* signature;
	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuff);
	if (FAILED(hr))
	{
		errorMsg = L"Failed create D3D12SerializeRootSignature";
		OutputDebugStringA((char*)errorBuff->GetBufferPointer());
		return false;
	}

	hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateRootSignature";
		return false;
	}

	// -- create vertex and pixel shaders -- //

	// when debugging, we can compile the shader files in runtime.
	// but for release versions, we can compile the hlsl shaders
	// with fxc.exe to create .cso files, which contains the shader bytexode.
	// we can load the .cso files at runtime to get the shader bytecode,
	// which of course is faster then compiling them at runtime

	// compile vertex shader
	ID3DBlob* vertexShader; // d3d blob for holding vertex shader bytecode
	hr = D3DCompileFromFile(L"VertexShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&vertexShader,
		&errorBuff);
	if (FAILED(hr))
	{
		errorMsg = L"Failed create D3DCompileFromFile (VertexShader.hlsl)";
		OutputDebugStringA((char*)errorBuff->GetBufferPointer());
		return false;
	}

	// fill out a shader bytecode structure, which is basically just a pointer to the shader bytecode and the size of the shader bytecode
	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
	vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

	// compile pixel shader
	ID3DBlob* pixelShader;
	hr = D3DCompileFromFile(L"PixelShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&pixelShader,
		&errorBuff);
	if (FAILED(hr))
	{
		errorMsg = L"Failed create D3DCompileFromFile (PixelShader.hlsl)";
		OutputDebugStringA((char*)errorBuff->GetBufferPointer());
		Running = false;
		return false;
	}

	// fill out shader bytecode structure for pixel shader
	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
	pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
	pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();

	// -- create input layout -- //

	// the input layout is used by the input assembler so that it knows how to read the vertexdata bound to it
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	// fill out the input layout description structure
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

	// we can get the number of elements in an array by "sizeof(array) / sizeof(arrayElementType)"
	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	// -- create a pipeline state object (pso) -- //

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {}; // a structure to define a pso
	psoDesc.InputLayout = inputLayoutDesc; // the structure describing out input layout
	psoDesc.pRootSignature = rootSignature; // the root signature that describes the input data this pso needs
	psoDesc.VS = vertexShaderBytecode; // structure describing where to find the vertex shader bytecode and how large it is
	psoDesc.PS = pixelShaderBytecode; // same as VS but for pixel shader
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // type of topology we are drawing
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the render target
	psoDesc.SampleDesc = sampleDesc; // must be the same sample description as the swapchain and depth/stencil buffer
	psoDesc.SampleMask = 0xffffffff; // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // a default rasterizer state
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // a default blend state
	psoDesc.NumRenderTargets = 1; // we are only binding one render target
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // a default depth stencil state

	// create the pso
	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateGraphicsPipelineState";
		return false;
	}

	// text pso
	ID3DBlob* textVertexShader;
	hr = D3DCompileFromFile(
		L"TextVertexShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&textVertexShader,
		&errorBuff
	);
	if (FAILED(hr))
	{
		errorMsg = L"Failed create D3DCompileFromFile (TextVertexShader.hlsl)";
		OutputDebugStringA((char*)errorBuff->GetBufferPointer());
		Running = false;
		return false;
	}

	D3D12_SHADER_BYTECODE textVertexShaderBytecode = {};
	textVertexShaderBytecode.BytecodeLength = textVertexShader->GetBufferSize();
	textVertexShaderBytecode.pShaderBytecode = textVertexShader->GetBufferPointer();

	ID3DBlob* textPixelShader;
	hr = D3DCompileFromFile(
		L"TextPixelShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&textPixelShader,
		&errorBuff
	);
	if (FAILED(hr))
	{
		errorMsg = L"Failed create D3DCompileFromFile (TextPixelShader.hlsl)";
		OutputDebugStringA((char*)errorBuff->GetBufferPointer());
		Running = false;
		return false;
	}

	D3D12_SHADER_BYTECODE textPixelShaderBytecode = {};
	textPixelShaderBytecode.BytecodeLength = textPixelShader->GetBufferSize();
	textPixelShaderBytecode.pShaderBytecode = textPixelShader->GetBufferPointer();

	// create input layout
	D3D12_INPUT_ELEMENT_DESC textInputLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1}
	};

	D3D12_INPUT_LAYOUT_DESC textInputLayoutDesc = {};
	textInputLayoutDesc.NumElements = sizeof(textInputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	textInputLayoutDesc.pInputElementDescs = textInputLayout;

	// create text pipeline state object
	D3D12_GRAPHICS_PIPELINE_STATE_DESC textpsoDesc = {};
	textpsoDesc.InputLayout = textInputLayoutDesc;
	textpsoDesc.pRootSignature = rootSignature;
	textpsoDesc.VS = textVertexShaderBytecode;
	textpsoDesc.PS = textPixelShaderBytecode;
	textpsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	textpsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	textpsoDesc.SampleDesc = sampleDesc;
	textpsoDesc.SampleMask = 0xffffffff;
	textpsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	D3D12_BLEND_DESC textBlendStateDesc = {};
	textBlendStateDesc.AlphaToCoverageEnable = FALSE;
	textBlendStateDesc.IndependentBlendEnable = FALSE;
	textBlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	textBlendStateDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	textBlendStateDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	textBlendStateDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	textBlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	textBlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	textBlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	textBlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	textpsoDesc.BlendState = textBlendStateDesc;
	textpsoDesc.NumRenderTargets = 1;
	D3D12_DEPTH_STENCIL_DESC textDepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	textDepthStencilDesc.DepthEnable = false;
	textpsoDesc.DepthStencilState = textDepthStencilDesc;

	// create text pso
	hr = device->CreateGraphicsPipelineState(&textpsoDesc, IID_PPV_ARGS(&textPSO));
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateGraphicsPipelineState";
		Running = false;
		return false;
	}

	// -- create vertex buffer -- //

	// a triangle
	Vertex vList[] = {
		// front face
		{ -0.5f,  0.5f, -0.5f, 0.0f, 0.0f },
		{  0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
		{ -0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
		{  0.5f,  0.5f, -0.5f, 1.0f, 0.0f },

		// right side face
		{  0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
		{  0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
		{  0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
		{  0.5f,  0.5f, -0.5f, 0.0f, 0.0f },

		// left side face
		{ -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
		{ -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
		{ -0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
		{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f },

		// back face
		{  0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
		{ -0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
		{  0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
		{ -0.5f,  0.5f,  0.5f, 1.0f, 0.0f },

		// top face
		{ -0.5f,  0.5f, -0.5f, 0.0f, 1.0f },
		{  0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
		{  0.5f,  0.5f, -0.5f, 1.0f, 1.0f },
		{ -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },

		// bottom face
		{  0.5f, -0.5f,  0.5f, 0.0f, 0.0f },
		{ -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
		{  0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
		{ -0.5f, -0.5f,  0.5f, 1.0f, 0.0f },
	};

	int vBufferSize = sizeof(vList);

	// create default heap
    // default heap is memory on the gpu. only the gpu has access to this memory
    // to get data into this heap, we will have to uploadthe data using an upload heap
	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), // resource description for this buffer
		D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy desctination state since we will copy data
										// from the upload heap to this heap
		nullptr, // optimized clear value must be null for this type of resource, used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&vertexBuffer));
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateCommittedResource";
		Running = false;
		return false;
	}

	// we can give resource heaps a name so when we debug with the graphics debugger we know the resource we are looking for
	vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

	// create upload heap
	// upload heaps are used to upload data to the gpu. cpu can write to it, gpu can read from it
	// we will upload the vertex buffer using this heap to default heap
	ID3D12Resource* vBufferUploadHeap;
	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ, // gpu will read from this buffer and copy its contents to the default heap
		nullptr,
		IID_PPV_ARGS(&vBufferUploadHeap));
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateCommittedResource";
		Running = false;
		return false;
	}
	vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = reinterpret_cast<BYTE*>(vList); // pointer to our vertex array
	vertexData.RowPitch = vBufferSize; // size of all our triangle vertex data
	vertexData.SlicePitch = vBufferSize; // also the size of our triangle vertex data

    // we are now creating a command with the command list to copy the data from the upload heap tothe default heap
	UpdateSubresources(commandList, vertexBuffer, vBufferUploadHeap, 0, 0, 1, &vertexData);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// -- create index buffer -- //

	// a quad (2 triangles)
	DWORD iList[] = {
		// ffront face
		0, 1, 2, // first triangle
		0, 3, 1, // second triangle

		// left face
		4, 5, 6, // first triangle
		4, 7, 5, // second triangle

		// right face
		8, 9, 10, // first triangle
		8, 11, 9, // second triangle

		// back face
		12, 13, 14, // first triangle
		12, 15, 13, // second triangle

		// top face
		16, 17, 18, // first triangle
		16, 19, 17, // second triangle

		// bottom face
		20, 21, 22, // first triangle
		20, 23, 21, // second triangle
	};

	int iBufferSize = sizeof(iList);

	numCubeIndices = sizeof(iList) / sizeof(DWORD);

	// create default heap to hold index buffer
	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(iBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST, // start in the copy destination state
		nullptr, // optimized clear value must be null for this type of resource
		IID_PPV_ARGS(&indexBuffer));
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateCommittedResource";
		Running = false;
		return false;
	}

	// we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
	vertexBuffer->SetName(L"Index Buffer Resource Heap");

	// create upload heap to upload index buffer
	ID3D12Resource* iBufferUploadHeap;
	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
		nullptr,
		IID_PPV_ARGS(&iBufferUploadHeap));
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateCommittedResource";
		Running = false;
		return false;
	}
	vBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = reinterpret_cast<BYTE*>(iList); // pointer to our index array
	indexData.RowPitch = iBufferSize; // size of all our index buffer
	indexData.SlicePitch = iBufferSize; // also the size of our index buffer

	// we are now creating a command with the command list to copy the data from
	// the upload heap to the default heap
	UpdateSubresources(commandList, indexBuffer, iBufferUploadHeap, 0, 0, 1, &indexData);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// Create the depth/stencil buffer

	// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsDescriptorHeap));
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateDescriptorHeap";
		Running = false;
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, Width, Height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&depthStencilBuffer)
	);
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateCommittedResource";
		Running = false;
		return false;
	}
	dsDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

	device->CreateDepthStencilView(depthStencilBuffer, &depthStencilDesc, dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < frameBufferCount; ++i)
	{
		// create resource for cube 1
		hr = device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // this heap will be used to upload the constant buffer data
			D3D12_HEAP_FLAG_NONE, // no flags
			&CD3DX12_RESOURCE_DESC::Buffer(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT), // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
			D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
			nullptr, // we do not have use an optimized clear value for constant buffers
			IID_PPV_ARGS(&constantBufferUploadHeaps[i]));
		if (FAILED(hr))
		{
			errorMsg = L"Failed create CreateCommittedResource";
			Running = false;
			return false;
		}
		constantBufferUploadHeaps[i]->SetName(L"Constant Buffer Upload Resource Heap");

		ZeroMemory(&cbPerObject, sizeof(cbPerObject));

		CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)

		// map the resource heap to get a gpu virtual address to the beginning of the heap
		hr = constantBufferUploadHeaps[i]->Map(0, &readRange, reinterpret_cast<void**>(&cbvGPUAddress[i]));

		// Because of the constant read alignment requirements, constant buffer views must be 256 bit aligned. Our buffers are smaller than 256 bits,
		// so we need to add spacing between the two buffers, so that the second buffer starts at 256 bits from the beginning of the resource heap.
		memcpy(cbvGPUAddress[i], &cbPerObject, sizeof(cbPerObject)); // cube1's constant buffer data
		memcpy(cbvGPUAddress[i] + ConstantBufferPerObjectAlignedSize, &cbPerObject, sizeof(cbPerObject)); // cube2's constant buffer data
	}

	// load image from file
	D3D12_RESOURCE_DESC textureDesc;
	int imageBytesPerRow;
	BYTE* imageData;
	int imageSize = LoadImageDataFromFile(&imageData, textureDesc, L"image.jpg", imageBytesPerRow);
	if (imageSize <= 0)
	{
		errorMsg = L"Failed create LoadImageDataFromFile";
		Running = false;
		return false;
	}

	// create a default heap where the upload heap will copy its contents into (contents being the texture)
	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&textureBuffer));
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateCommittedResource";
		Running = false;
		return false;
	}
	textureBuffer->SetName(L"Texture Buffer Resource Heap");

	UINT64 textureUploadBufferSize;

	device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureBufferUploadHeap));
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateCommittedResource";
		Running = false;
		return false;
	}
	textureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

	// store texture data in upload heap
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &imageData[0];
	textureData.RowPitch = imageBytesPerRow;
	textureData.SlicePitch = imageBytesPerRow * textureDesc.Height;

	UpdateSubresources(commandList, textureBuffer, textureBufferUploadHeap, 0, 0, 1, &textureData);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	// create the descriptor heap that will store our srv
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mainDescriptorHeap));
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateDescriptorHeap";
		Running = false;
	}

	// create shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(textureBuffer, &srvDesc, mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// load font
	std::wstring dir = L"fnt/";
	std::wstring fontname = dir + L"jb_mono.fnt";
	jbFont = LoadFont(fontname.c_str(), Width, Height);
	std::wstring fontTexName = dir + jbFont.fontImage;
	D3D12_RESOURCE_DESC fontTextureDesc;
	int fontImageBytesPerRow;
	BYTE* fontImageData;
	int fontImageSize = LoadImageDataFromFile(&fontImageData, fontTextureDesc, fontTexName.c_str(), fontImageBytesPerRow);
	if (fontImageData <= 0)
	{
		errorMsg = L"Failed create LoadImageDataFromFile";
		Running = false;
		return false;
	}

	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&fontTextureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&jbFont.textureBuffer)
	);
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateCommittedResource";
		Running = false;
		return false;
	}

	jbFont.textureBuffer->SetName(L"Font Texture Buffer Resource Heap");

	ID3D12Resource* fontTextureBufferUploadHeap;
	UINT64 fontTextureUploadBufferSize;
	device->GetCopyableFootprints(&fontTextureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &fontTextureUploadBufferSize);

	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(fontTextureUploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&fontTextureBufferUploadHeap)
	);
	if (FAILED(hr))
	{
		errorMsg = L"Failed create CreateCommittedResource";
		Running = false;
		return false;
	}
	fontTextureBufferUploadHeap->SetName(L"Font Texture Buffer Upload Resource Heap");

	D3D12_SUBRESOURCE_DATA fontTextureData = {};
	fontTextureData.pData = &fontImageData[0];
	fontTextureData.RowPitch = fontImageBytesPerRow;
	fontTextureData.SlicePitch = fontImageBytesPerRow * fontTextureDesc.Height;

	UpdateSubresources(commandList, jbFont.textureBuffer, fontTextureBufferUploadHeap, 0, 0, 1, &fontTextureData);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(jbFont.textureBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	D3D12_SHADER_RESOURCE_VIEW_DESC fontsrvDesc = {};
	fontsrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	fontsrvDesc.Format = fontTextureDesc.Format;
	fontsrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	fontsrvDesc.Texture2D.MipLevels = 1;

	srvHandleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	jbFont.srvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), 1, srvHandleSize);

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 1, srvHandleSize);
	device->CreateShaderResourceView(jbFont.textureBuffer, &fontsrvDesc, srvHandle);

	// create text vertex buffer committed resources
	for (int i = 0; i < frameBufferCount; ++i)
	{
		ID3D12Resource* vBufferUploadHeap;
		hr = device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(maxNumTextCharacters * sizeof(TextVertex)),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&textVertexBuffer[i])
		);
		if (FAILED(hr))
		{
			errorMsg = L"Failed create CreateCommittedResource";
			Running = false;
			return false;
		}
		textVertexBuffer[i]->SetName(L"Text Vertex Buffer Upload Resource Heap");

		CD3DX12_RANGE readRange(0, 0);

		hr = textVertexBuffer[i]->Map(0, &readRange, reinterpret_cast<void**>(&textVBGPUAddress[i]));
	}

	// Now we execute the command list to upload the initial assets (triangle data)
	commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { commandList };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// increment the fence value now, otherwise the buffer might not be uploaded by the time we start drawing
	fenceValue[frameIndex]++;
	hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
	if (FAILED(hr))
	{
		errorMsg = L"Failed create Signal";
		Running = false;
		return false;
	}

	delete imageData;
	delete fontImageData;

	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = vBufferSize;

	for (int i = 0; i < frameBufferCount; ++i)
	{
		textVertexBufferView[i].BufferLocation = textVertexBuffer[i]->GetGPUVirtualAddress();
		textVertexBufferView[i].StrideInBytes = sizeof(TextVertex);
		textVertexBufferView[i].SizeInBytes = maxNumTextCharacters * sizeof(TextVertex);
	}

	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
	indexBufferView.SizeInBytes = iBufferSize;

	// Fill out the Viewport
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Width;
	viewport.Height = Height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Fill out a scissor rect
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = Width;
	scissorRect.bottom = Height;

	// build projection and view matrix
	XMMATRIX tmpMat = XMMatrixPerspectiveFovLH(45.0f * (3.14f / 180.0f), (float)Width / (float)Height, 0.1f, 1000.0f);
	XMStoreFloat4x4(&cameraProjMat, tmpMat);

	// set starting camera state
	cameraPosition = XMFLOAT4(0.0f, 2.0f, -4.0f, 0.0f);
	cameraTarget = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	cameraUp = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

	// build view matrix
	XMVECTOR cPos = XMLoadFloat4(&cameraPosition);
	XMVECTOR cTarg = XMLoadFloat4(&cameraTarget);
	XMVECTOR cUp = XMLoadFloat4(&cameraUp);
	tmpMat = XMMatrixLookAtLH(cPos, cTarg, cUp);
	XMStoreFloat4x4(&cameraViewMat, tmpMat);

	// set starting cubes position
	// first cube
	cube1Position = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // set cube 1's position
	XMVECTOR posVec = XMLoadFloat4(&cube1Position); // create xmvector for cube1's position

	tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube1's position vector
	XMStoreFloat4x4(&cube1RotMat, XMMatrixIdentity()); // initialize cube1's rotation matrix to identity matrix
	XMStoreFloat4x4(&cube1WorldMat, tmpMat); // store cube1's world matrix

	// second cube
	cube2PositionOffset = XMFLOAT4(1.5f, 0.0f, 0.0f, 0.0f);
	posVec = XMLoadFloat4(&cube2PositionOffset) + XMLoadFloat4(&cube1Position); // create xmvector for cube2's position
																				// we are rotating around cube1 here, so add cube2's position to cube1

	tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube2's position offset vector
	XMStoreFloat4x4(&cube2RotMat, XMMatrixIdentity()); // initialize cube2's rotation matrix to identity matrix
	XMStoreFloat4x4(&cube2WorldMat, tmpMat); // store cube2's world matrix

	return true;
}

void Update(double delta)
{
	// update app logic, such as moving the camera or figuring out what objects are in view

	// create rotation matrix
	XMMATRIX rotXMat = XMMatrixRotationX(0.0001f * delta);
	XMMATRIX rotYMat = XMMatrixRotationY(0.0002f * delta);
	XMMATRIX rotZMat = XMMatrixRotationZ(0.0003f * delta);

	// add rotation to cube1
	XMMATRIX rotMat = XMLoadFloat4x4(&cube1RotMat) * rotXMat * rotYMat * rotZMat;
	XMStoreFloat4x4(&cube1RotMat, rotMat);

	XMMATRIX translationMat = XMMatrixTranslationFromVector(XMLoadFloat4(&cube1Position));

	XMMATRIX worldMat = rotMat * translationMat;

	XMStoreFloat4x4(&cube1WorldMat, worldMat);

	// update constant buffer for cube1
	XMMATRIX viewMat = XMLoadFloat4x4(&cameraViewMat);
	XMMATRIX projMat = XMLoadFloat4x4(&cameraProjMat);
	XMMATRIX wvpMat = XMLoadFloat4x4(&cube1WorldMat) * viewMat * projMat;
	XMMATRIX transposed = XMMatrixTranspose(wvpMat);
	XMStoreFloat4x4(&cbPerObject.wvpMat, transposed);

	memcpy(cbvGPUAddress[frameIndex], &cbPerObject, sizeof(cbPerObject));

	// now do cube2's world matrix
	// create rotation matrices for cube2
	rotXMat = XMMatrixRotationX(0.0003f * delta);
	rotYMat = XMMatrixRotationY(0.0002f * delta);
	rotZMat = XMMatrixRotationZ(0.0001f * delta);

	rotMat = rotZMat * (XMLoadFloat4x4(&cube2RotMat) * (rotXMat * rotYMat));
	XMStoreFloat4x4(&cube2RotMat, rotMat);

	XMMATRIX translationOffsetMat = XMMatrixTranslationFromVector(XMLoadFloat4(&cube2PositionOffset));

	XMMATRIX scaleMat = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	worldMat = scaleMat * translationOffsetMat * rotMat * translationMat;

	wvpMat = XMLoadFloat4x4(&cube2WorldMat) * viewMat * projMat; // create wvp matrix
	transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&cbPerObject.wvpMat, transposed); // store transposed wvp matrix in constant buffer

	memcpy(cbvGPUAddress[frameIndex] + ConstantBufferPerObjectAlignedSize, &cbPerObject, sizeof(cbPerObject));

	XMStoreFloat4x4(&cube2WorldMat, worldMat);
}

void UpdatePipeline()
{
	HRESULT hr;

	// We have to wait for the gpu to finish with the command allocator before we reset it
	WaitForPreviousFrame();

	// we can only reset an allocator once the gpu is done with it
	// resetting an allocator frees the memory that the command list was stored in
	hr = commandAllocator[frameIndex]->Reset();
	if (FAILED(hr))
	{
		errorMsg = L"Failed Reset";
		Running = false;
	}

	// reset the command list. by resetting the command list we are putting it into
	// a recording state so we can start recording commands into the command allocator.
	// the command allocator that we reference here may have multiple command lists
	// associated with it, but only one can be recording at any time. Make sure
	// that any other command lists associated to this command allocator are in
	// the closed state (not recording).
	// Here you will pass an initial pipeline state object as the second parameter,
	// but in this tutorial we are only clearing the rtv, and do not actually need
	// anything but an initial default pipeline, which is what we get by setting
	// the second parameter to NULL
	hr = commandList->Reset(commandAllocator[frameIndex], pipelineStateObject);
	if (FAILED(hr))
	{
		errorMsg = L"Failed Reset";
		Running = false;
	}

	// here we start recording commands into the commandList (which all the commands will be stored in the commandAllocator)

	// transition the "frameIndex" render target from the present state to the render target state so the command list draws to it starting from here
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// here we again get the handle to our current render target view so we can set it as the render target in the output merger stage of the pipeline
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

	// get a handle to the depth/stencil buffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// set the render target for the output merger stage (the output of the pipeline)
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Clear the render target by using the ClearRenderTargetView command
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// clear the depth/stencil buffer
	commandList->ClearDepthStencilView(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// set root signature
	commandList->SetGraphicsRootSignature(rootSignature); // set the root signature

	// set the descriptor heap
	ID3D12DescriptorHeap* descriptorHeaps[] = { mainDescriptorHeap };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	commandList->SetGraphicsRootDescriptorTable(1, mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// draw triangle
	commandList->RSSetViewports(1, &viewport); // set the viewports
	commandList->RSSetScissorRects(1, &scissorRect); // set the scissor rects
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // set the primitive topology
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // set the vertex buffer (using the vertex buffer view)
	commandList->IASetIndexBuffer(&indexBufferView);

	// first cube

	// set cube1's constant buffer
	commandList->SetGraphicsRootConstantBufferView(0, constantBufferUploadHeaps[frameIndex]->GetGPUVirtualAddress());

	// draw first cube
	commandList->DrawIndexedInstanced(numCubeIndices, 1, 0, 0, 0);

	// second cube

	// set cube2's constant buffer. You can see we are adding the size of ConstantBufferPerObject to the constant buffer
	// resource heaps address. This is because cube1's constant buffer is stored at the beginning of the resource heap, while
	// cube2's constant buffer data is stored after (256 bits from the start of the heap).
	commandList->SetGraphicsRootConstantBufferView(0, constantBufferUploadHeaps[frameIndex]->GetGPUVirtualAddress() + ConstantBufferPerObjectAlignedSize);

	// draw second cube
	commandList->DrawIndexedInstanced(numCubeIndices, 1, 0, 0, 0);

	RenderText(jbFont, std::wstring(L"FPS: ") + std::to_wstring(timer.fps), XMFLOAT2(0.02f, 0.01f), XMFLOAT2(2.0f, 2.0f));

	// transition the "frameIndex" render target from the render target state to the present state. If the debug layer is enabled, you will receive a
	// warning if present is called on the render target when it's not in the present state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	hr = commandList->Close();
	if (FAILED(hr))
	{
		errorMsg = L"Failed Close";
		Running = false;
	}
}

void Render()
{
	HRESULT hr;

	UpdatePipeline(); // update the pipeline by sending commands to the commandqueue

	// create an array of command lists (only one command list here)
	ID3D12CommandList* ppCommandLists[] = { commandList };

	// execute the array of command lists
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// this command goes in at the end of our command queue. we will know when our command queue
	// has finished because the fence value will be set to "fenceValue"from the gpu since command queue is being executed on the gpu
	hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
	if (FAILED(hr))
	{
		errorMsg = L"Failed Signal";
		Running = false;
	}

	// present the current backbuffer
	hr = swapChain->Present(0, 0);
	if (FAILED(hr))
	{
		errorMsg = L"Failed Present";
		Running = false;
	}
}

void Cleanup()
{
	// wait for the gpu to finish all frames
	for (int i = 0; i < frameBufferCount; ++i)
	{
		frameIndex = i;
		WaitForPreviousFrame();
	}

	// get swapchain out of full screen before exiting
	BOOL fs = false;
	if (swapChain->GetFullscreenState(&fs, NULL))
		swapChain->SetFullscreenState(false, NULL);

	SAFE_RELEASE(device);
	SAFE_RELEASE(swapChain);
	SAFE_RELEASE(commandQueue);
	SAFE_RELEASE(rtvDescriptorHeap);
	SAFE_RELEASE(commandList);

	for (int i = 0; i < frameBufferCount; ++i)
	{
		SAFE_RELEASE(renderTargets[i]);
		SAFE_RELEASE(commandAllocator[i]);
		SAFE_RELEASE(fence[i]);
	};

	SAFE_RELEASE(pipelineStateObject);
	SAFE_RELEASE(rootSignature);
	SAFE_RELEASE(vertexBuffer);

	SAFE_RELEASE(indexBuffer);

	SAFE_RELEASE(depthStencilBuffer);
	SAFE_RELEASE(dsDescriptorHeap);

	for (int i = 0; i < frameBufferCount; ++i)
	{
		SAFE_RELEASE(constantBufferUploadHeaps[i]);
	};
}

void WaitForPreviousFrame()
{
	HRESULT hr;

	// swap the current rtv buffer index so we draw on the correct buffer
	frameIndex = swapChain->GetCurrentBackBufferIndex();

	// if the current fence value is still less than "fenceValue", then we know the GPU has not finished executing
	// the command queue since it has not reached the "commandQueue->Signal(fence, fenceValue)" command
	if (fence[frameIndex]->GetCompletedValue() < fenceValue[frameIndex])
	{
		// we have the fence create an event which is signaled once the fence's current value is "fenceValue"
		hr = fence[frameIndex]->SetEventOnCompletion(fenceValue[frameIndex], fenceEvent);
		if (FAILED(hr))
		{
			errorMsg = L"Failed SetEventOnCompletion";
			Running = false;
		}

		// We will wait until the fence has triggered the event that it's current value has reached "fenceValue". once it's value
		// has reached "fenceValue", we know the command queue has finished executing
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// increment fenceValue for next frame
	fenceValue[frameIndex]++;
}

Font LoadFont(LPCWSTR filename, int windowWidth, int windowHeight)
{
	std::wifstream fs;
	fs.open(filename);

	Font font;
	std::wstring tmp;
	int startpos;

	// extract font name
	fs >> tmp >> tmp; // info face="JetBrains Mono Regular"
	while (tmp.find_first_of(L"\"") == tmp.find_last_of(L"\""))
	{
		std::wstring saved = tmp;
		fs >> tmp;
		tmp = saved + L' ' + tmp;
	}
	startpos = tmp.find(L"\"") + 1;
	font.name = tmp.substr(startpos, tmp.size() - startpos - 1);

	// get font size
	fs >> tmp; // size=73
	startpos = tmp.find(L"=") + 1;
	font.size = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// get bold, italic...
	fs >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp; // bold=0 italic=0 charset="" unicode=0 stretchH=100 smooth=1 aa=1

	// get padding
	fs >> tmp; // padding=5,5,5,5 spacing=0,0
	startpos = tmp.find(L"=") + 1;
	tmp = tmp.substr(startpos, tmp.size() - startpos); // 5,5,5,5

	// get up padding
	startpos = tmp.find(L",") + 1;
	font.toppadding = std::stoi(tmp.substr(0, startpos)) / (float)windowWidth;

	// get right padding
	tmp = tmp.substr(startpos, tmp.size() - startpos);
	startpos = tmp.find(L",") + 1;
	font.rightpadding = std::stoi(tmp.substr(0, startpos)) / (float)windowWidth;

	// get down padding
	tmp = tmp.substr(startpos, tmp.size() - startpos);
	startpos = tmp.find(L",") + 1;
	font.bottompadding = std::stoi(tmp.substr(0, startpos)) / (float)windowWidth;

	// get left padding
	tmp = tmp.substr(startpos, tmp.size() - startpos);
	font.leftpadding = std::stoi(tmp) / (float)windowWidth;

	fs >> tmp; // spacing=0,0

	// get lineheight (how much to move down for each line), and normalize (between 0.0 and 1.0 based on size of font)
	fs >> tmp >> tmp; // common lineHeight=95
	startpos = tmp.find(L"=") + 1;
	font.lineHeight = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowHeight;

	// get base height (height of all characters), and normalize (between 0.0 and 1.0 based on size of font)
	fs >> tmp; // base=68
	startpos = tmp.find(L"=") + 1;
	font.baseHeight = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowHeight;

	// get texture width
	fs >> tmp; // scaleW=512
	startpos = tmp.find(L"=") + 1;
	font.textureWidth = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// get texture height
	fs >> tmp; // scaleH=512
	startpos = tmp.find(L"=") + 1;
	font.textureHeight = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// get pages, packed, page id
	fs >> tmp >> tmp; // pages=1 packed=0
	fs >> tmp >> tmp; // page id=0

	// get texture filename
	std::wstring wtmp;
	fs >> wtmp; // file="jb_mono.png"
	startpos = wtmp.find(L"\"") + 1;
	font.fontImage = wtmp.substr(startpos, wtmp.size() - startpos - 1);

	// get number of characters
	fs >> tmp >> tmp; // chars count=97
	startpos = tmp.find(L"=") + 1;
	font.numCharacters = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	font.CharList = new FontChar[font.numCharacters];

	for (int c = 0; c < font.numCharacters; ++c)
	{
		// get unicode id
		fs >> tmp >> tmp; // char id=0
		startpos = tmp.find(L"=") + 1;
		font.CharList[c].id = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

		// get x
		fs >> tmp; // x=392
		startpos = tmp.find(L"=") + 1;
		font.CharList[c].u = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)font.textureWidth;

		// get y
		fs >> tmp; // y=340
		startpos = tmp.find(L"=") + 1;
		font.CharList[c].v = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)font.textureHeight;

		// get width
		fs >> tmp; // width=47
		startpos = tmp.find(L"=") + 1;
		tmp = tmp.substr(startpos, tmp.size() - startpos);
		font.CharList[c].width = (float)std::stoi(tmp) / (float)windowWidth;
		font.CharList[c].twidth = (float)std::stoi(tmp) / (float)font.textureWidth;

		// get height
		fs >> tmp; // height=57
		startpos = tmp.find(L"=") + 1;
		tmp = tmp.substr(startpos, tmp.size() - startpos);
		font.CharList[c].height = (float)std::stoi(tmp) / (float)windowHeight;
		font.CharList[c].theight = (float)std::stoi(tmp) / (float)font.textureHeight;

		// get xoffset
		fs >> tmp; // xoffset=-6
		startpos = tmp.find(L"=") + 1;
		font.CharList[c].xoffset = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowWidth;

		// get yoffset
		fs >> tmp; // yoffset=16
		startpos = tmp.find(L"=") + 1;
		font.CharList[c].yoffset = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowHeight;

		// get xadvance
		fs >> tmp; // xadvance=65
		startpos = tmp.find(L"=") + 1;
		font.CharList[c].xadvance = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowWidth;

		// get page
		// get channel
		fs >> tmp >> tmp; // page=0    chnl=0
	}

	// get number of kernings
	fs >> tmp >> tmp; // kernings count=96
	startpos = tmp.find(L"=") + 1;
	font.numKernings = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// initialize the kernings list
	font.KerningsList = new FontKerning[font.numKernings];

	for (int k = 0; k < font.numKernings; ++k)
	{
		// get first character
		fs >> tmp >> tmp; // kerning first=87
		startpos = tmp.find(L"=") + 1;
		font.KerningsList[k].firstId = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

		// get second character
		fs >> tmp; // second=45
		startpos = tmp.find(L"=") + 1;
		font.KerningsList[k].secondId = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

		// get amount
		fs >> tmp; // amount=-1
		startpos = tmp.find(L"=") + 1;
		int t = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos));
		font.KerningsList[k].amount = (float)t / (float)windowWidth;
	}

	return font;
}

void RenderText(Font font, std::wstring text, XMFLOAT2 pos, XMFLOAT2 scale, XMFLOAT2 padding, XMFLOAT4 color)
{
	commandList->ClearDepthStencilView(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	commandList->SetPipelineState(textPSO);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	commandList->IASetVertexBuffers(0, 1, &textVertexBufferView[frameIndex]);
	commandList->SetGraphicsRootDescriptorTable(1, font.srvHandle);

	int numCharacters = 0;
	float topLeftScreenX = (pos.x * 2.0f) - 1.0f;
	float topLeftScreenY = ((1.0f - pos.y) * 2.0f) - 1.0f;
	float x = topLeftScreenX;
	float y = topLeftScreenY;
	float horrizontalPadding = (font.leftpadding + font.rightpadding) * padding.x;
	float verticalPadding = (font.toppadding + font.bottompadding) * padding.y;

	TextVertex* vert = (TextVertex*)textVBGPUAddress[frameIndex];
	wchar_t lastChar = -1;

	for (int i = 0; i < text.size(); ++i)
	{
		wchar_t c = text[i];
		FontChar* fc = font.GetChar(c);
		if (fc == nullptr) continue;
		if (c == L'\0') break;
		if (c == L'\n')
		{
			x = topLeftScreenX;
			y -= (font.lineHeight + verticalPadding) * scale.y;
			continue;
		}

		if (numCharacters >= maxNumTextCharacters) break;

		float kerning = 0.0f;
		if (i > 0)
			kerning = font.GetKerning(lastChar, c);

		vert[numCharacters] = TextVertex(
			color.x,
			color.y,
			color.z,
			color.w,
			fc->u,
			fc->v,
			fc->twidth,
			fc->theight,
			x + ((fc->xoffset + kerning) * scale.x),
			y - (fc->yoffset * scale.y),
			fc->width * scale.x,
			fc->height * scale.y
		);

		numCharacters++;

		x += (fc->xadvance - horrizontalPadding) * scale.x;

		lastChar = c;
	}

	commandList->DrawInstanced(4, numCharacters, 0, 0);
}

// load and decode image from file
int LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow)
{
	HRESULT hr;

	static IWICImagingFactory *wicFactory;

	IWICBitmapDecoder *wicDecoder = NULL;
	IWICBitmapFrameDecode *wicFrame = NULL;
	IWICFormatConverter *wicConverter = NULL;

	bool imageConverted = false;

	if (wicFactory == NULL)
	{
		CoInitialize(NULL);

		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&wicFactory)
		);
		if (FAILED(hr)) return 0;
	}

	hr = wicFactory->CreateDecoderFromFilename(
		filename,
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&wicDecoder
	);
	if (FAILED(hr)) return 0;

	hr = wicDecoder->GetFrame(0, &wicFrame);
	if (FAILED(hr)) return 0;

	WICPixelFormatGUID pixelFormat;
	hr = wicFrame->GetPixelFormat(&pixelFormat);
	if (FAILED(hr)) return 0;

	UINT textureWidth, textureHeight;
	hr = wicFrame->GetSize(&textureWidth, &textureHeight);
	if (FAILED(hr)) return 0;

	DXGI_FORMAT dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);

	if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
	{
		WICPixelFormatGUID convertToPixelFormat = GetConvertToWICFormat(pixelFormat);
		if (convertToPixelFormat == GUID_WICPixelFormatDontCare) return 0;

		dxgiFormat = GetDXGIFormatFromWICFormat(convertToPixelFormat);

		hr = wicFactory->CreateFormatConverter(&wicConverter);
		if (FAILED(hr)) return 0;

		BOOL canConvert = FALSE;
		hr = wicConverter->CanConvert(pixelFormat, convertToPixelFormat, &canConvert);
		if (FAILED(hr) || !canConvert) return 0;

		hr = wicConverter->Initialize(wicFrame, convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
		if (FAILED(hr)) return 0;

		imageConverted = true;
	}

	int bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat);
	bytesPerRow = (textureWidth * bitsPerPixel) / 8;
	int imageSize = bytesPerRow * textureHeight;

	*imageData = (BYTE*)malloc(imageSize);

	if (imageConverted)
	{
		hr = wicConverter->CopyPixels(0, bytesPerRow, imageSize, *imageData);
		if (FAILED(hr)) return 0;
	}
	else
	{
		hr = wicFrame->CopyPixels(0, bytesPerRow, imageSize, *imageData);
		if (FAILED(hr)) return 0;
	}

	resourceDescription = {};
	resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDescription.Alignment = 0;
	resourceDescription.Width = textureWidth;
	resourceDescription.Height = textureHeight;
	resourceDescription.DepthOrArraySize = 1;
	resourceDescription.MipLevels = 1;
	resourceDescription.Format = dxgiFormat;
	resourceDescription.SampleDesc.Count = 1;
	resourceDescription.SampleDesc.Quality = 0;
	resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

	return imageSize;
}

DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID)
{
	if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat) return DXGI_FORMAT_R32G32B32A32_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf) return DXGI_FORMAT_R16G16B16A16_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBA) return DXGI_FORMAT_R16G16B16A16_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA) return DXGI_FORMAT_R8G8B8A8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGRA) return DXGI_FORMAT_B8G8R8A8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR) return DXGI_FORMAT_B8G8R8X8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102) return DXGI_FORMAT_R10G10B10A2_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGRA5551) return DXGI_FORMAT_B5G5R5A1_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR565) return DXGI_FORMAT_B5G6R5_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFloat) return DXGI_FORMAT_R32_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayHalf) return DXGI_FORMAT_R16_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGray) return DXGI_FORMAT_R16_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppGray) return DXGI_FORMAT_R8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppAlpha) return DXGI_FORMAT_A8_UNORM;

	else return DXGI_FORMAT_UNKNOWN;
}

// get a dxgi compatible wic format from another wic format
WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID)
{
	if (wicFormatGUID == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
	else if (wicFormatGUID == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif

	else return GUID_WICPixelFormatDontCare;
}

// get the number of bits per pixel for a dxgi format
int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat)
{
	if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
	else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
	else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
	else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;

	else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT) return 32;
	else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R16_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R8_UNORM) return 8;
	else if (dxgiFormat == DXGI_FORMAT_A8_UNORM) return 8;
}

LRESULT CALLBACK WndProc(HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)

{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			if (MessageBox(0, L"Are you sure you want to exit?",
				L"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				Running = false;
				DestroyWindow(hwnd);
			}
		}
		return 0;

	case WM_DESTROY: // x button on top right corner of window was pressed
		Running = false;
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd,
		msg,
		wParam,
		lParam);
}

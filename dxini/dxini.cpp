// dxini.cpp : Определяет точку входа для приложения.
//

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

#include "framework.h"
#include "dxini.h"
#include <dxgi1_4.h>
#include <dxgi.h>

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
void WaitForPreiousFrame(); //wait until gpu is finished with command list
// ===============

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Разместите код здесь.

    // initalize direct3d
    if (!InitD3D()) {
        MessageBox(0, L"Failed to initalize direct3d 12", L"Error", MB_OK);
        Cleanup();
        return 1;
    }


    // we want to wait for the gpu to finish executing the command list before we start releasing everything
    WaitForPreiousFrame();

    // close the fence event
    CloseHandle(fenceEvent);

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DXINI, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DXINI));

    MSG msg;

    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            ;
        }
    }

    return (int) msg.wParam;
}

bool InitD3D() 
{
    HRESULT hr;

    HRESULT WINAPI D3D12CreateDevice(
        _In_opt_  IUnknown          *pAdapter,
                  D3D_FEATURE_LEVEL MinimumFeatureLevel,
        _In_      REFIID            riid,
        _Out_opt_ void              **ppDevice
    );

    // -- create the device -- //

    IDXGIFactory4* dxgiFactory;
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) {
        return false;

    }

    IDXGIAdapter1* adapter; // adapters are the graphic card (this included the embedded graphics on the motherboard)
    int adapterIndex = 0; // we'll start looking for directx 12 compatible graphics devices starting at index 0
    bool adapterFound = 0;

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

        // we want a device that is compatible with direct3s 12 (feature level 11 or higher)
        hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr);
        if (SUCCEEDED(hr))
        {
            MessageBox(0, L"Adapter found", L"Adapter", MB_OK);
            adapterFound = true;
            break;
        }

        adapterIndex++;
    }

    if (!adapterFound)
    {
        MessageBox(0, L"Adapter not found", L"Adapter", MB_OK);
        return false;
    }

    return true;
}

void WaitForPreiousFrame() 
{

}

void Cleanup() 
{

}


//
//  ФУНКЦИЯ: MyRegisterClass()
//
//  ЦЕЛЬ: Регистрирует класс окна.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DXINI));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(nullptr);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   ФУНКЦИЯ: InitInstance(HINSTANCE, int)
//
//   ЦЕЛЬ: Сохраняет маркер экземпляра и создает главное окно
//
//   КОММЕНТАРИИ:
//
//        В этой функции маркер экземпляра сохраняется в глобальной переменной, а также
//        создается и выводится главное окно программы.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

   HWND hWnd = CreateWindowW(
       szWindowClass, 
       szTitle, 
       WS_OVERLAPPEDWINDOW | WS_VISIBLE,
       CW_USEDEFAULT, 
       0, 
       CW_USEDEFAULT, 
       0, 
       nullptr, 
       nullptr, 
       hInstance, 
       nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  ФУНКЦИЯ: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ЦЕЛЬ: Обрабатывает сообщения в главном окне.
//
//  WM_COMMAND  - обработать меню приложения
//  WM_PAINT    - Отрисовка главного окна
//  WM_DESTROY  - отправить сообщение о выходе и вернуться
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Разобрать выбор в меню:
            switch (wmId)
            {
            /*case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;*/
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Добавьте сюда любой код прорисовки, использующий HDC...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Обработчик сообщений для окна "О программе".
//INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
//{
//    UNREFERENCED_PARAMETER(lParam);
//    switch (message)
//    {
//    case WM_INITDIALOG:
//        return (INT_PTR)TRUE;
//
//    case WM_COMMAND:
//        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
//        {
//            EndDialog(hDlg, LOWORD(wParam));
//            return (INT_PTR)TRUE;
//        }
//        break;
//    }
//    return (INT_PTR)FALSE;
//}

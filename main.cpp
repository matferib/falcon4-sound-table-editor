// Please keep headers sorted.
#include <iostream>
#include <memory>
#include <string>

#define NOMINMAX
#include <d3d11.h>
#include <Windows.h>
#include <strsafe.h>

#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "imgui.h"

// Forward declare message handler from imgui_impl_win32.cpp (outside of anonymous namespace). See imgui_impl_win32.h.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {

// The UI abstraction is contained here. Subclasses implement the look and feel.
class GenericUI {
public:
  virtual ~GenericUI() {}

  // Fires up any initialization of the UI and runs until the program ends.
  virtual void Run() = 0;
};

// TODO(seifer): move this to a separate module to avoid polluting the main one.
// An example class implementing GenericUI in command line style (CommandLineUI).
class CommandLineUI : public GenericUI {
public:
  ~CommandLineUI() override {}

  void Run() override {
    enum Options {
      OPTION_QUIT = 1
    };
    do {
      std::cout << "Please, select an option:" << std::endl;
      std::cout << "1: quit." << std::endl;
      int option_selected;
      std::cin >> option_selected;
      if (std::cin.eof()) {
        break;
      }
      if (!std::cin) {
        std::cout << "Bad input..." << std::endl;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        continue;
      }
      if (option_selected == OPTION_QUIT) {
        break;
      }
      std::cout << "You chose option: " << option_selected << ", unknown" << std::endl;
    } while (true);
  }
};

// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// TODO(sassah): implement a windows UI (WindowUI) that inherits from GenericUI.
class WindowUI : public GenericUI {
public:
  static constexpr auto kAppName = L"Falcon XXX Editor";

  ~WindowUI() override {}

  // Helper functions
  static bool CreateDeviceD3D(HWND hWnd) {
      // Setup swap chain
      DXGI_SWAP_CHAIN_DESC sd;
      ZeroMemory(&sd, sizeof(sd));
      sd.BufferCount = 2;
      sd.BufferDesc.Width = 0;
      sd.BufferDesc.Height = 0;
      sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      sd.BufferDesc.RefreshRate.Numerator = 60;
      sd.BufferDesc.RefreshRate.Denominator = 1;
      sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      sd.OutputWindow = hWnd;
      sd.SampleDesc.Count = 1;
      sd.SampleDesc.Quality = 0;
      sd.Windowed = TRUE;
      sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

      UINT createDeviceFlags = 0;
      //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
      D3D_FEATURE_LEVEL featureLevel;
      const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
      if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK) {
          return false;
      }

      CreateRenderTarget();
      return true;
  }

  static void CleanupDeviceD3D() {
      CleanupRenderTarget();
      if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
      if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
      if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
  }

  static void CreateRenderTarget() {
      ID3D11Texture2D* pBackBuffer;
      g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
      g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
      pBackBuffer->Release();
  }

  static void CleanupRenderTarget() {
      if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
  }

  static LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam)) { return true; }

    RECT rcClient;
    int i;

    switch (uMsg) {
    case WM_CREATE: // creating main window  
        return 0;
    case WM_SIZE:   // main window changed size 
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
        return 0;
    case WM_CLOSE:
        // Create the message box. If the user clicks 
        // the Yes button, destroy the main window. 
        if (MessageBox(hwnd, L"Close?", kAppName, MB_YESNOCANCEL) == IDYES) {
            DestroyWindow(hwnd);
        } else {
            return 0;
        }
    case WM_DESTROY:
        // Post the WM_QUIT message to 
        // quit the application terminate. 
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
  }

  // Routine to display windows messages. Must be called right after the error because relies on GetLastError.
  void DisplayError() const {
      LPVOID lpMsgBuf;
      LPVOID lpDisplayBuf;
      DWORD dw = GetLastError();

      FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER |
          FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL,
          dw,
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          (LPTSTR)&lpMsgBuf,
          0, NULL);

      lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
          (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)kAppName) + 40) * sizeof(TCHAR));
      StringCchPrintf((LPTSTR)lpDisplayBuf,
          LocalSize(lpDisplayBuf) / sizeof(TCHAR),
          TEXT("%s failed with error %d: %s"),
          L"Run", dw, lpMsgBuf);
      MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

      LocalFree(lpMsgBuf);
      LocalFree(lpDisplayBuf);
  }

  void Run() override {
    const wchar_t CLASS_NAME[] = L"Sample Window Class";

    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = (HINSTANCE)GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window.
    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        kAppName,                       // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        wc.hInstance,  // Instance handle
        NULL        // Additional application data
    );
    if (hwnd == nullptr) {
        DisplayError();
        return;
    }

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        DisplayError();
        return;
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable some options
    // Initialize Platform + Renderer backends (here: using imgui_impl_win32.cpp + imgui_impl_dx11.cpp)
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Event loop.
    // Main loop
    bool done = false;
    while (!done) {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) { done = true; }
        }
        if (done) {
            break;        
        }

        DoImGuiFrame();
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
  }

  void DoImGuiFrame() {
      // Start the Dear ImGui frame
      ImGui_ImplDX11_NewFrame();
      ImGui_ImplWin32_NewFrame();
      ImGui::NewFrame();

      if (show_demo_window_) {
          ImGui::ShowDemoWindow(&show_demo_window_);
      }

      // Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
      {
          static float f = 0.0f;
          static int counter = 0;

          ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

          ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
          ImGui::Checkbox("Demo Window", &show_demo_window_);      // Edit bools storing our window open/close state
          ImGui::Checkbox("Another Window", &show_another_window_);

          ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
          ImGui::ColorEdit3("clear color", (float*)&clear_color_); // Edit 3 floats representing a color

          if (ImGui::Button("Button")) {                           // Buttons return true when clicked (most widgets return true when edited/activated)
              counter++;
          }
          ImGui::SameLine();
          ImGui::Text("counter = %d", counter);

          ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
          ImGui::End();
      }

      // 3. Show another simple window.
      if (show_another_window_) {
          ImGui::Begin("Another Window", &show_another_window_);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
          ImGui::Text("Hello from another window!");
          if (ImGui::Button("Close Me")) {
              show_another_window_ = false;
          }
          ImGui::End();
      }

      // Rendering
      ImGui::Render();
      const float clear_color_with_alpha[4] = { clear_color_.x * clear_color_.w, clear_color_.y * clear_color_.w, clear_color_.z * clear_color_.w, clear_color_.w };
      g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
      g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
      ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

      g_pSwapChain->Present(0, 0); // Present without vsync
  }

 private:
  bool show_demo_window_ = true;
  bool show_another_window_ = false;
  ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
};


// This is the main UI entry (factory). It is the one which picks which UI it should start.
std::unique_ptr<GenericUI> CreateUI(int argc, char** argv) {
  // TODO(sassah): add you UI constructor here.
  return std::make_unique<WindowUI>();
}

}  // namespace


int main(int argc, char** argv) {
  auto ifg = CreateUI(argc, argv);
  ifg->Run();
  return 0;
}
#include "Hooks_Present.h"
#include "../../Menu/Menu.h"
#include <D3DX11.h>
#include <d3d11.h>
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "d3d11.lib")

#include "../../ImGui/imgui_impl_dx11.h"
#include "../../ImGui/imgui_impl_win32.h"
#include "../Minhook/include/MinHook.h"

typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (Menu::IsVisible)
    {
        // Passa o controle para o ImGui quando o menu está visível
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return true;

        // Intercepta inputs de mouse e teclado para o menu
        if (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_MOUSEMOVE)
            return true;
    }

    // Caso contrário, repassa a chamada para o WndProc original
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!KieroHooks::Imgui_Init)
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
        {
            pDevice->GetImmediateContext(&pContext);

            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            window = sd.OutputWindow;

            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();

            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

            KieroHooks::InitImgui();
            Menu::Style();

            KieroHooks::Imgui_Init = true;
        }
        else
            return oPresent(pSwapChain, SyncInterval, Flags);
    }

    // Ativar/Desativar Menu com tecla Insert
    if (GetAsyncKeyState(VK_INSERT) & 1)
    {
        Menu::IsVisible = !Menu::IsVisible;

        // Controla a exibição do cursor e desbloqueia o movimento quando o menu é ativado/desativado
        ImGui::GetIO().MouseDrawCursor = Menu::IsVisible;

        if (Menu::IsVisible)
        {
            // Mostra o cursor e desbloqueia o controle
            ShowCursor(TRUE);
            ClipCursor(NULL);  // Libera o cursor para que ele possa se mover livremente
        }
        else
        {
            // Esconde o cursor e permite que o jogo recupere o controle
            ShowCursor(FALSE);

            // Restaurar o bloqueio do cursor conforme necessário pelo jogo
            RECT rect;
            GetClientRect(window, &rect);
            MapWindowPoints(window, NULL, (POINT*)&rect, 2);
            ClipCursor(&rect);  // Bloqueia o cursor novamente dentro da janela do jogo
        }
    }

    // Inicia uma nova frame para o ImGui
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Renderiza o menu apenas se ele estiver visível
    if (Menu::IsVisible)
    {
        Menu::Render();
    }

    // Finaliza a frame do ImGui e instrui o DirectX a renderizar
    ImGui::EndFrame();
    ImGui::Render();
    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(pSwapChain, SyncInterval, Flags);
}

void KieroHooks::Init()
{
    bool should_load = false;
    do
    {
        if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
        {
            kiero::bind(8, reinterpret_cast<void**>(&oPresent), reinterpret_cast<void*>(hkPresent));
            should_load = true;
        }

    } while (should_load == false);
}

void KieroHooks::Shutdown()
{
    MH_DisableHook(MH_ALL_HOOKS);
}

void KieroHooks::InitImgui()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange; // Não deixa o ImGui mudar o cursor do sistema
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(pDevice, pContext);
}

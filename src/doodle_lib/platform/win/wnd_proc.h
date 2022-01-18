//
// Created by TD on 2022/1/18.
//

#pragma once
#include <doodle_lib/platform/win/windows_alias.h>

namespace doodle::win {

static ID3D11Device* g_pd3dDevice                     = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext       = nullptr;
static IDXGISwapChain* g_pSwapChain                   = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
}  // namespace doodle::win

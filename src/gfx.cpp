#include "gfx.h"

#include <d3d11.h>
#include <dxgi.h>
#include <wincodec.h>
#include <stdlib.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

extern "C" {
void igNewFrame(void);
ImDrawData *igGetDrawData(void);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static ID3D11Device *g_device;
static ID3D11DeviceContext *g_context;
static IDXGISwapChain *g_swap;
static ID3D11RenderTargetView *g_target;
static int g_width;
static int g_height;

static ID3D11Texture2D *g_page;
static ID3D11ShaderResourceView *g_page_view;
static int g_page_w;
static int g_page_h;
static unsigned long long g_page_id;

#define GFX_THUMB_SLOTS 24

static ID3D11Texture2D *g_thumb_texture[GFX_THUMB_SLOTS];
static ID3D11ShaderResourceView *g_thumb_view[GFX_THUMB_SLOTS];
static int g_thumb_w[GFX_THUMB_SLOTS];
static int g_thumb_h[GFX_THUMB_SLOTS];
static unsigned long long g_thumb_id[GFX_THUMB_SLOTS];

static void release_target(void)
{
	if (g_target) {
		g_target->Release();
		g_target = NULL;
	}
}

static int make_target(void)
{
	ID3D11Texture2D *back = NULL;
	HRESULT hr;
	if (!g_swap)
		return 0;
	hr = g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&back);
	if (FAILED(hr) || !back)
		return 0;
	hr = g_device->CreateRenderTargetView(back, NULL, &g_target);
	back->Release();
	return SUCCEEDED(hr);
}

static int create_device(HWND hwnd)
{
	DXGI_SWAP_CHAIN_DESC desc;
	D3D_FEATURE_LEVEL level;
	D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
	UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	HRESULT hr;

	RECT rc;
	GetClientRect(hwnd, &rc);
	g_width = rc.right - rc.left;
	g_height = rc.bottom - rc.top;
	if (g_width < 1)
		g_width = 1;
	if (g_height < 1)
		g_height = 1;

	ZeroMemory(&desc, sizeof(desc));
	desc.BufferCount = 2;
	desc.BufferDesc.Width = g_width;
	desc.BufferDesc.Height = g_height;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferDesc.RefreshRate.Numerator = 60;
	desc.BufferDesc.RefreshRate.Denominator = 1;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.OutputWindow = hwnd;
	desc.SampleDesc.Count = 1;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, levels, 2,
		D3D11_SDK_VERSION, &desc, &g_swap, &g_device, &level, &g_context);
	if (FAILED(hr)) {
		hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, flags, levels, 2,
			D3D11_SDK_VERSION, &desc, &g_swap, &g_device, &level, &g_context);
	}
	if (FAILED(hr))
		return 0;
	make_target();
	return g_target != NULL;
}

static void destroy_device(void)
{
	gfx_thumbs_reset();
	if (g_page_view) {
		g_page_view->Release();
		g_page_view = NULL;
	}
	if (g_page) {
		g_page->Release();
		g_page = NULL;
	}
	g_page_id = 0;
	g_page_w = 0;
	g_page_h = 0;
	release_target();
	if (g_swap) {
		g_swap->Release();
		g_swap = NULL;
	}
	if (g_context) {
		g_context->Release();
		g_context = NULL;
	}
	if (g_device) {
		g_device->Release();
		g_device = NULL;
	}
}

static int save_png(const wchar_t *path, const unsigned char *pixels, int width, int height, int stride)
{
	IWICImagingFactory *factory = NULL;
	IWICBitmapEncoder *encoder = NULL;
	IWICStream *stream = NULL;
	IWICBitmapFrameEncode *frame = NULL;
	IPropertyBag2 *props = NULL;
	WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
	unsigned char *bgra;
	size_t total;
	int i;
	HRESULT hr;
	int ok = 0;

	total = (size_t)stride * (size_t)height;
	bgra = (unsigned char *)malloc(total);
	if (!bgra)
		return 0;
	for (i = 0; i < (int)total; i += 4) {
		bgra[i] = pixels[i + 2];
		bgra[i + 1] = pixels[i + 1];
		bgra[i + 2] = pixels[i];
		bgra[i + 3] = pixels[i + 3];
	}
	pixels = bgra;

	hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&factory));
	if (FAILED(hr)) {
		free(bgra);
		return 0;
	}
	hr = factory->CreateStream(&stream);
	if (SUCCEEDED(hr))
		hr = stream->InitializeFromFilename(path, GENERIC_WRITE);
	if (SUCCEEDED(hr))
		hr = factory->CreateEncoder(GUID_ContainerFormatPng, NULL, &encoder);
	if (SUCCEEDED(hr))
		hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
	if (SUCCEEDED(hr))
		hr = encoder->CreateNewFrame(&frame, &props);
	if (SUCCEEDED(hr))
		hr = frame->Initialize(props);
	if (SUCCEEDED(hr))
		hr = frame->SetSize((UINT)width, (UINT)height);
	if (SUCCEEDED(hr))
		hr = frame->SetPixelFormat(&format);
	if (SUCCEEDED(hr))
		hr = frame->WritePixels((UINT)height, (UINT)stride, (UINT)(stride * height), (BYTE *)pixels);
	if (SUCCEEDED(hr))
		hr = frame->Commit();
	if (SUCCEEDED(hr))
		hr = encoder->Commit();
	if (SUCCEEDED(hr))
		ok = 1;

	if (props)
		props->Release();
	if (frame)
		frame->Release();
	if (encoder)
		encoder->Release();
	if (stream)
		stream->Release();
	if (factory)
		factory->Release();
	free(bgra);
	return ok;
}

static int capture(const wchar_t *path)
{
	ID3D11Texture2D *back = NULL;
	ID3D11Texture2D *staging = NULL;
	D3D11_TEXTURE2D_DESC desc;
	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT hr;
	int ok = 0;

	hr = g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&back);
	if (FAILED(hr) || !back)
		return 0;
	back->GetDesc(&desc);
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.MiscFlags = 0;
	desc.SampleDesc.Count = 1;
	if (SUCCEEDED(g_device->CreateTexture2D(&desc, NULL, &staging))) {
		g_context->CopyResource(staging, back);
		if (SUCCEEDED(g_context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped))) {
			ok = save_png(path, (const unsigned char *)mapped.pData,
				(int)desc.Width, (int)desc.Height, (int)mapped.RowPitch);
			g_context->Unmap(staging, 0);
		}
	}
	if (staging)
		staging->Release();
	back->Release();
	return ok;
}

int gfx_init(HWND hwnd)
{
	ImGuiIO *io;

	if (!create_device(hwnd))
		return 0;
	ImGui::CreateContext();
	io = &ImGui::GetIO();
	io->IniFilename = NULL;
	io->LogFilename = NULL;
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	if (!ImGui_ImplWin32_Init(hwnd)) {
		destroy_device();
		ImGui::DestroyContext();
		return 0;
	}
	if (!ImGui_ImplDX11_Init(g_device, g_context)) {
		ImGui_ImplWin32_Shutdown();
		destroy_device();
		ImGui::DestroyContext();
		return 0;
	}
	return 1;
}

void gfx_shutdown(void)
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	destroy_device();
}

void gfx_new_frame(void)
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	igNewFrame();
}

int gfx_render(int r, int g, int b, const wchar_t *capture_path)
{
	const float clear[4] = { (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f };
	int captured = 0;

	if (!g_context || !g_target)
		return 0;
	g_context->OMSetRenderTargets(1, &g_target, NULL);
	g_context->ClearRenderTargetView(g_target, clear);
	ImGui_ImplDX11_RenderDrawData(igGetDrawData());
	if (capture_path)
		captured = capture(capture_path);
	if (g_swap)
		g_swap->Present(capture_path ? 0 : 1, 0);
	return captured;
}

int gfx_resize(int width, int height)
{
	if (width < 1)
		width = 1;
	if (height < 1)
		height = 1;
	if (!g_swap)
		return 0;
	if (width == g_width && height == g_height && g_target)
		return 1;
	g_width = width;
	g_height = height;
	release_target();
	if (FAILED(g_swap->ResizeBuffers(0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, 0))) {
		if (FAILED(g_swap->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0)))
			return 0;
	}
	return make_target();
}

int gfx_page_texture(const unsigned char *bgra, int width, int height, int stride)
{
	D3D11_TEXTURE2D_DESC desc;
	D3D11_SUBRESOURCE_DATA data;
	ID3D11Texture2D *texture = NULL;
	ID3D11ShaderResourceView *view = NULL;
	HRESULT hr;

	if (!g_device || !bgra || width < 1 || height < 1)
		return 0;
	if (g_page && width == g_page_w && height == g_page_h) {
		g_context->UpdateSubresource(g_page, 0, NULL, bgra, (UINT)stride, 0);
		return 1;
	}

	ZeroMemory(&desc, sizeof(desc));
	desc.Width = (UINT)width;
	desc.Height = (UINT)height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	ZeroMemory(&data, sizeof(data));
	data.pSysMem = bgra;
	data.SysMemPitch = (UINT)stride;
	hr = g_device->CreateTexture2D(&desc, &data, &texture);
	if (FAILED(hr) || !texture)
		return 0;
	hr = g_device->CreateShaderResourceView(texture, NULL, &view);
	if (FAILED(hr) || !view) {
		texture->Release();
		return 0;
	}
	if (g_page_view)
		g_page_view->Release();
	if (g_page)
		g_page->Release();
	g_page = texture;
	g_page_view = view;
	g_page_id = (unsigned long long)(uintptr_t)view;
	g_page_w = width;
	g_page_h = height;
	return 1;
}

unsigned long long gfx_page_texture_id(void)
{
	return g_page_id;
}

int gfx_thumb_texture(int slot, const unsigned char *bgra, int width, int height, int stride)
{
	D3D11_TEXTURE2D_DESC desc;
	D3D11_SUBRESOURCE_DATA data;
	ID3D11Texture2D *texture = NULL;
	ID3D11ShaderResourceView *view = NULL;
	HRESULT hr;

	if (!g_device || !bgra || slot < 0 || slot >= GFX_THUMB_SLOTS ||
		width < 1 || height < 1)
		return 0;
	if (g_thumb_texture[slot] && g_thumb_w[slot] == width && g_thumb_h[slot] == height) {
		g_context->UpdateSubresource(g_thumb_texture[slot], 0, NULL, bgra, (UINT)stride, 0);
		g_thumb_id[slot] = (unsigned long long)(uintptr_t)g_thumb_view[slot];
		return 1;
	}

	ZeroMemory(&desc, sizeof(desc));
	desc.Width = (UINT)width;
	desc.Height = (UINT)height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	ZeroMemory(&data, sizeof(data));
	data.pSysMem = bgra;
	data.SysMemPitch = (UINT)stride;
	hr = g_device->CreateTexture2D(&desc, &data, &texture);
	if (FAILED(hr) || !texture)
		return 0;
	hr = g_device->CreateShaderResourceView(texture, NULL, &view);
	if (FAILED(hr) || !view) {
		texture->Release();
		return 0;
	}
	if (g_thumb_view[slot])
		g_thumb_view[slot]->Release();
	if (g_thumb_texture[slot])
		g_thumb_texture[slot]->Release();
	g_thumb_texture[slot] = texture;
	g_thumb_view[slot] = view;
	g_thumb_w[slot] = width;
	g_thumb_h[slot] = height;
	g_thumb_id[slot] = (unsigned long long)(uintptr_t)view;
	return 1;
}

unsigned long long gfx_thumb_texture_id(int slot)
{
	if (slot < 0 || slot >= GFX_THUMB_SLOTS)
		return 0;
	return g_thumb_id[slot];
}

void gfx_thumbs_reset(void)
{
	int i;
	for (i = 0; i < GFX_THUMB_SLOTS; i++) {
		if (g_thumb_view[i]) {
			g_thumb_view[i]->Release();
			g_thumb_view[i] = NULL;
		}
		if (g_thumb_texture[i]) {
			g_thumb_texture[i]->Release();
			g_thumb_texture[i] = NULL;
		}
		g_thumb_w[i] = 0;
		g_thumb_h[i] = 0;
		g_thumb_id[i] = 0;
	}
}

int gfx_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam) ? 1 : 0;
}

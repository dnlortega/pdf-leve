#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <objbase.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "app.h"
#include "gfx.h"
#include "mu.h"
#include "ocr.h"
#include "resource.h"
#include "ui_imgui.h"

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

static App g_app;
static int g_shot_mode;
static int g_shot_done;
static wchar_t g_shot_path[1024];
static int g_shot_width = 1280;
static int g_shot_height = 860;
static int g_force_dark = -1;
static int g_start_page = -1;
static wchar_t g_start_doc[1024];

static int scaled(int value, int dpi)
{
	return MulDiv(value, dpi > 0 ? dpi : 96, 96);
}

static void save_window_placement(void)
{
	WINDOWPLACEMENT wp;
	if (!g_app.hwnd)
		return;
	memset(&wp, 0, sizeof(wp));
	wp.length = sizeof(wp);
	if (GetWindowPlacement(g_app.hwnd, &wp)) {
		g_app.state.maximized = wp.showCmd == SW_SHOWMAXIMIZED;
		g_app.state.window = wp.rcNormalPosition;
	} else if (!IsIconic(g_app.hwnd)) {
		GetWindowRect(g_app.hwnd, &g_app.state.window);
	}
}

static int rect_visible(const RECT *rect)
{
	MONITORINFO info;
	HMONITOR monitor;
	RECT overlap;
	if (rect->right - rect->left < 320 || rect->bottom - rect->top < 240)
		return 0;
	monitor = MonitorFromRect(rect, MONITOR_DEFAULTTONULL);
	if (!monitor)
		return 0;
	info.cbSize = sizeof(info);
	if (!GetMonitorInfoW(monitor, &info))
		return 0;
	if (!IntersectRect(&overlap, rect, &info.rcWork))
		return 0;
	return overlap.right - overlap.left >= 200 && overlap.bottom - overlap.top >= 120;
}

static void initial_rect(const State *boot, RECT *out)
{
	RECT work;
	HDC dc;
	int dpi = 96;
	int width;
	int height;

	SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
	if (work.right - work.left < 320)
		work.right = work.left + 1280;
	if (work.bottom - work.top < 240)
		work.bottom = work.top + 800;

	dc = GetDC(NULL);
	if (dc) {
		dpi = GetDeviceCaps(dc, LOGPIXELSX);
		ReleaseDC(NULL, dc);
	}
	if (dpi <= 0)
		dpi = 96;

	if (g_shot_mode) {
		width = g_shot_width;
		height = g_shot_height;
	} else {
		width = scaled(1180, dpi);
		height = scaled(800, dpi);
	}

	if (!g_shot_mode && rect_visible(&boot->window)) {
		*out = boot->window;
		if (out->right - out->left > work.right - work.left)
			out->right = out->left + (work.right - work.left);
		if (out->bottom - out->top > work.bottom - work.top)
			out->bottom = out->top + (work.bottom - work.top);
		return;
	}

	if (width > (work.right - work.left) * 9 / 10)
		width = (work.right - work.left) * 9 / 10;
	if (height > (work.bottom - work.top) * 9 / 10)
		height = (work.bottom - work.top) * 9 / 10;
	out->left = work.left + ((work.right - work.left) - width) / 2;
	out->top = work.top + ((work.bottom - work.top) - height) / 2;
	out->right = out->left + width;
	out->bottom = out->top + height;
}

static void parse_args(void)
{
	int argc = 0;
	int i;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (!argv)
		return;
	for (i = 1; i < argc; i++) {
		const wchar_t *arg = argv[i];
		if (wcscmp(arg, L"--shot") == 0 && i + 1 < argc) {
			g_shot_mode = 1;
			wcsncpy(g_shot_path, argv[++i], 1023);
			g_shot_path[1023] = 0;
		} else if (wcscmp(arg, L"--size") == 0 && i + 1 < argc) {
			int w = 0;
			int h = 0;
			if (swscanf(argv[++i], L"%dx%d", &w, &h) == 2 && w > 320 && h > 240) {
				g_shot_width = w;
				g_shot_height = h;
			}
		} else if (wcscmp(arg, L"--page") == 0 && i + 1 < argc) {
			g_start_page = _wtoi(argv[++i]) - 1;
			if (g_start_page < 0)
				g_start_page = 0;
		} else if (wcscmp(arg, L"--dark") == 0) {
			g_force_dark = 1;
		} else if (wcscmp(arg, L"--light") == 0) {
			g_force_dark = 0;
		} else if (arg[0] != L'-' && !g_start_doc[0]) {
			wcsncpy(g_start_doc, arg, 1023);
			g_start_doc[1023] = 0;
		}
	}
	LocalFree(argv);
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (gfx_wndproc(hwnd, msg, wparam, lparam))
		return 1;

	switch (msg) {
	case WM_NCCALCSIZE:
		if (wparam)
			return 0;
		break;
	case WM_NCHITTEST: {
		POINT pt;
		RECT rc;
		int dpi;
		int border;
		int left;
		int right;
		int top;
		int bottom;
		pt.x = GET_X_LPARAM(lparam);
		pt.y = GET_Y_LPARAM(lparam);
		GetWindowRect(hwnd, &rc);
		dpi = g_app.dpi > 0 ? g_app.dpi : 96;
		border = scaled(6, dpi);
		left = pt.x >= rc.left && pt.x < rc.left + border;
		right = pt.x < rc.right && pt.x >= rc.right - border;
		top = pt.y >= rc.top && pt.y < rc.top + border;
		bottom = pt.y < rc.bottom && pt.y >= rc.bottom - border;
		if (top && left)
			return HTTOPLEFT;
		if (top && right)
			return HTTOPRIGHT;
		if (bottom && left)
			return HTBOTTOMLEFT;
		if (bottom && right)
			return HTBOTTOMRIGHT;
		if (left)
			return HTLEFT;
		if (right)
			return HTRIGHT;
		if (top)
			return HTTOP;
		if (bottom)
			return HTBOTTOM;
		return HTCLIENT;
	}
	case WM_GETMINMAXINFO: {
		MINMAXINFO *info = (MINMAXINFO *)lparam;
		MONITORINFO monitor;
		HMONITOR handle = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		int dpi = g_app.dpi > 0 ? g_app.dpi : 96;
		monitor.cbSize = sizeof(monitor);
		if (GetMonitorInfoW(handle, &monitor)) {
			info->ptMaxPosition.x = monitor.rcWork.left - monitor.rcMonitor.left;
			info->ptMaxPosition.y = monitor.rcWork.top - monitor.rcMonitor.top;
			info->ptMaxSize.x = monitor.rcWork.right - monitor.rcWork.left;
			info->ptMaxSize.y = monitor.rcWork.bottom - monitor.rcWork.top;
			info->ptMaxTrackSize.x = info->ptMaxSize.x;
			info->ptMaxTrackSize.y = info->ptMaxSize.y;
		}
		info->ptMinTrackSize.x = scaled(760, dpi);
		info->ptMinTrackSize.y = scaled(560, dpi);
		return 0;
	}
	case WM_SIZE:
		if (wparam != SIZE_MINIMIZED) {
			int width = LOWORD(lparam);
			int height = HIWORD(lparam);
			g_app.width = width;
			g_app.height = height;
			gfx_resize(width, height);
			if (g_app.doc.doc && g_app.doc.fit)
				g_app.render_pending = 1;
		}
		return 0;
	case WM_DPICHANGED: {
		RECT *suggested = (RECT *)lparam;
		int dpi = HIWORD(wparam) ? HIWORD(wparam) : LOWORD(wparam);
		if (dpi > 0) {
			g_app.dpi = dpi;
			g_app.ui = (float)dpi / 96.0f;
		}
		SetWindowPos(hwnd, NULL, suggested->left, suggested->top,
			suggested->right - suggested->left, suggested->bottom - suggested->top,
			SWP_NOZORDER | SWP_NOACTIVATE);
		if (g_app.doc.doc)
			g_app.render_pending = 1;
		return 0;
	}
	case WM_DROPFILES: {
		HDROP drop = (HDROP)wparam;
		UINT count = DragQueryFileW(drop, 0xFFFFFFFF, NULL, 0);
		if (count > 0) {
			wchar_t path[1024];
			if (DragQueryFileW(drop, 0, path, 1024))
				app_open_path(&g_app, path);
		}
		DragFinish(drop);
		SetForegroundWindow(hwnd);
		return 0;
	}
	case WM_APP_OCR_DONE:
		app_ocr_done(&g_app, (OcrResult *)lparam);
		return 0;
	case WM_CLOSE:
		save_window_placement();
		DestroyWindow(hwnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_ERASEBKGND:
		return 1;
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static int shot_ready(void)
{
	if (!g_shot_mode || g_shot_done)
		return 0;
	if (g_app.frame_count < 8)
		return 0;
	if (!g_app.doc.doc)
		return 1;
	return !g_app.render_pending && g_app.tex_page == g_app.doc.page;
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE previous, PWSTR command, int show)
{
	WNDCLASSEXW wc;
	State boot;
	RECT rect;
	HWND hwnd;
	HICON icon;
	HICON icon_small;
	HRESULT com;
	MSG msg;
	int running = 1;
	(void)previous;
	(void)command;
	(void)show;

	parse_args();
	com = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (!mu_load()) {
		const wchar_t *detail = mu_error();
		wchar_t text[1200];
		_snwprintf(text, 1199,
			L"Não foi possível carregar o motor de PDF.\n\n%s\n\nMantenha mupdfcpp64.dll ao lado do executável.",
			detail && detail[0] ? detail : L"Biblioteca ausente.");
		text[1199] = 0;
		MessageBoxW(NULL, text, APP_NAME, MB_ICONERROR | MB_OK);
		if (SUCCEEDED(com))
			CoUninitialize();
		return 1;
	}

	state_load(&boot);
	initial_rect(&boot, &rect);

	icon = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON, 0, 0,
		LR_DEFAULTSIZE | LR_SHARED);
	icon_small = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);

	memset(&wc, 0, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = wnd_proc;
	wc.hInstance = inst;
	wc.hIcon = icon;
	wc.hIconSm = icon_small;
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszClassName = APP_CLASS;
	if (!RegisterClassExW(&wc)) {
		DWORD error = GetLastError();
		if (error != ERROR_CLASS_ALREADY_EXISTS) {
			MessageBoxW(NULL, L"Não foi possível registrar a janela.", APP_NAME,
				MB_ICONERROR | MB_OK);
			mu_unload();
			if (SUCCEEDED(com))
				CoUninitialize();
			return 1;
		}
	}

	hwnd = CreateWindowExW(WS_EX_APPWINDOW, APP_CLASS, APP_NAME,
		WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU |
		WS_CLIPCHILDREN,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, inst, NULL);
	if (!hwnd) {
		MessageBoxW(NULL, L"Não foi possível criar a janela.", APP_NAME,
			MB_ICONERROR | MB_OK);
		mu_unload();
		if (SUCCEEDED(com))
			CoUninitialize();
		return 1;
	}

	app_init(&g_app, inst, hwnd);
	if (g_force_dark >= 0)
		g_app.state.dark = g_force_dark;
	if (g_shot_mode)
		g_app.saving_disabled = 1;

	if (!gfx_init(hwnd)) {
		MessageBoxW(NULL, L"Não foi possível iniciar o Direct3D 11.", APP_NAME,
			MB_ICONERROR | MB_OK);
		app_shutdown(&g_app);
		DestroyWindow(hwnd);
		mu_unload();
		if (SUCCEEDED(com))
			CoUninitialize();
		return 1;
	}
	ui_init(&g_app);
	DragAcceptFiles(hwnd, TRUE);
	{
		int preference = 2;
		DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference,
			sizeof(preference));
	}

	ShowWindow(hwnd, boot.maximized && !g_shot_mode ? SW_SHOWMAXIMIZED : SW_SHOW);
	UpdateWindow(hwnd);
	SetForegroundWindow(hwnd);
	if (g_shot_mode)
		SetWindowPos(hwnd, NULL, rect.left, rect.top, rect.right - rect.left,
			rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE);

	if (g_start_doc[0]) {
		app_open_path(&g_app, g_start_doc);
	} else if (!g_shot_mode && boot.last_file[0] &&
		GetFileAttributesW(boot.last_file) != INVALID_FILE_ATTRIBUTES) {
		app_load_document(&g_app, boot.last_file, NULL);
	}
	if (g_start_page >= 0 && g_app.doc.doc)
		app_goto_page(&g_app, g_start_page);

	while (running) {
		const wchar_t *capture = NULL;
		int clear_r;
		int clear_g;
		int clear_b;
		int captured;

		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				running = 0;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		if (!running)
			break;

		gfx_new_frame();
		ui_frame(&g_app);
		app_render_tick(&g_app);
		app_search_tick(&g_app);

		ui_clear_color(&g_app, &clear_r, &clear_g, &clear_b);
		if (shot_ready())
			capture = g_shot_path;
		captured = gfx_render(clear_r, clear_g, clear_b, capture);
		if (capture && captured) {
			g_shot_done = 1;
			PostMessageW(hwnd, WM_CLOSE, 0, 0);
		}
		g_app.frame_count++;
		if (g_shot_mode && !g_shot_done && g_app.frame_count > 900)
			PostMessageW(hwnd, WM_CLOSE, 0, 0);
	}

	DragAcceptFiles(hwnd, FALSE);
	ui_shutdown(&g_app);
	gfx_shutdown();
	app_shutdown(&g_app);
	if (IsWindow(hwnd))
		DestroyWindow(hwnd);
	mu_unload();
	if (SUCCEEDED(com))
		CoUninitialize();
	return 0;
}

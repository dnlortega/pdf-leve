#include <windows.h>
#include <shellapi.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#ifndef IM_COL32
#define IM_COL32(r, g, b, a) \
	(((ImU32)(a) << 24) | ((ImU32)(b) << 16) | ((ImU32)(g) << 8) | (ImU32)(r))
#endif

#include "ui_imgui.h"
#include "gfx.h"
#include "theme.h"
#include "mu.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

typedef struct UiTheme {
	ImU32 chrome;
	ImU32 surface;
	ImU32 canvas;
	ImU32 ink;
	ImU32 muted;
	ImU32 border;
	ImU32 border_soft;
	ImU32 accent;
	ImU32 accent_hover;
	ImU32 on_accent;
	ImU32 accent_soft;
	ImU32 hover;
	ImU32 active;
	ImU32 close_hover;
	ImU32 search;
	ImU32 selection;
	ImU32 tip_bg;
	ImU32 tip_fg;
	ImU32 shadow;
	ImU32 field;
	ImU32 dim;
	ImU32 error;
} UiTheme;

static const UiTheme THEME_LIGHT = {
	IM_COL32(0xF3, 0xEF, 0xE6, 255),
	IM_COL32(0xFF, 0xFD, 0xF9, 255),
	IM_COL32(0xD6, 0xCF, 0xC2, 255),
	IM_COL32(0x2C, 0x28, 0x23, 255),
	IM_COL32(0x7A, 0x72, 0x65, 255),
	IM_COL32(0xDE, 0xD6, 0xC8, 255),
	IM_COL32(0xE7, 0xDF, 0xD2, 255),
	IM_COL32(0xB4, 0x68, 0x1A, 255),
	IM_COL32(0x8E, 0x4E, 0x10, 255),
	IM_COL32(0xFF, 0xFF, 0xFF, 255),
	IM_COL32(0xF1, 0xE2, 0xCC, 255),
	IM_COL32(0xEA, 0xE3, 0xD6, 255),
	IM_COL32(0xDF, 0xD5, 0xC3, 255),
	IM_COL32(0xC7, 0x44, 0x36, 255),
	IM_COL32(0xF0, 0xB8, 0x3C, 255),
	IM_COL32(0xC9, 0x83, 0x2E, 255),
	IM_COL32(0x33, 0x2E, 0x28, 255),
	IM_COL32(0xF7, 0xF2, 0xE9, 255),
	IM_COL32(0x9A, 0x90, 0x80, 255),
	IM_COL32(0xFF, 0xFF, 0xFF, 255),
	IM_COL32(0x1E, 0x1A, 0x16, 110),
	IM_COL32(0xA5, 0x3A, 0x2C, 255),
};

static const UiTheme THEME_DARK = {
	IM_COL32(0x26, 0x23, 0x1F, 255),
	IM_COL32(0x30, 0x2C, 0x27, 255),
	IM_COL32(0x19, 0x17, 0x14, 255),
	IM_COL32(0xF1, 0xEB, 0xE1, 255),
	IM_COL32(0xA6, 0x9B, 0x8C, 255),
	IM_COL32(0x3D, 0x37, 0x2F, 255),
	IM_COL32(0x34, 0x2F, 0x28, 255),
	IM_COL32(0xD9, 0x8A, 0x3C, 255),
	IM_COL32(0xE8, 0x9A, 0x4A, 255),
	IM_COL32(0x24, 0x1F, 0x1A, 255),
	IM_COL32(0x3B, 0x2C, 0x1A, 255),
	IM_COL32(0x38, 0x33, 0x2C, 255),
	IM_COL32(0x45, 0x3E, 0x34, 255),
	IM_COL32(0xC7, 0x44, 0x36, 255),
	IM_COL32(0xF0, 0xB8, 0x3C, 255),
	IM_COL32(0xD9, 0x8A, 0x3C, 255),
	IM_COL32(0x3A, 0x34, 0x2C, 255),
	IM_COL32(0xF1, 0xEB, 0xE1, 255),
	IM_COL32(0x00, 0x00, 0x00, 255),
	IM_COL32(0x22, 0x1F, 0x1B, 255),
	IM_COL32(0x00, 0x00, 0x00, 155),
	IM_COL32(0xE0, 0x82, 0x73, 255),
};

typedef struct Glyphs {
	char open[5];
	char search[5];
	char prev[5];
	char next[5];
	char zoom_out[5];
	char zoom_in[5];
	char fit[5];
	char rotate[5];
	char copy[5];
	char ocr[5];
	char close[5];
	char close_small[5];
	char min[5];
	char max[5];
	char restore[5];
	char doc[5];
	char moon[5];
	char sun[5];
	char info[5];
	char mark[5];
	char plus[5];
	char sparkle[5];
	char sidebar[5];
	char settings[5];
	char export_txt[5];
} Glyphs;

typedef struct UiRect {
	float x0, y0, x1, y1;
} UiRect;

static App *A;
static ImDrawList *DL;
static const UiTheme *TH;
static float U;
static float T;
static Glyphs G;
static UiRect g_view;
static int g_modal;

static char g_last_query[256];
static int g_jump_guard;
static wchar_t *g_ocr_seen;
static int g_password_was_open;
static int g_ocr_was_open;
static int g_ai_was_open;
static int g_styled_dark = -1;
static float g_styled_ui = -1.0f;
static int g_styled_text = -1;
static int g_about_open;
static int g_splash_force;
static int g_settings_open;
static int g_settings_age;
static UiRect g_settings_btn;

static ImVec2_c v2(float x, float y)
{
	ImVec2_c v;
	v.x = x;
	v.y = y;
	return v;
}

static UiRect rectf(float x0, float y0, float x1, float y1)
{
	UiRect r;
	r.x0 = x0;
	r.y0 = y0;
	r.x1 = x1;
	r.y1 = y1;
	return r;
}

static int S(int value)
{
	return (int)lroundf((float)value * U);
}

static ImU32 alpha_color(ImU32 color, int alpha)
{
	return (color & 0x00FFFFFFu) | ((ImU32)alpha << 24);
}

static ImVec4_c color4(ImU32 color)
{
	ImVec4_c out;
	out.x = (float)(color & 0xFF) / 255.0f;
	out.y = (float)((color >> 8) & 0xFF) / 255.0f;
	out.z = (float)((color >> 16) & 0xFF) / 255.0f;
	out.w = (float)((color >> 24) & 0xFF) / 255.0f;
	return out;
}

static int in_rect(UiRect r, ImVec2_c p)
{
	return p.x >= r.x0 && p.x < r.x1 && p.y >= r.y0 && p.y < r.y1;
}

static ImVec2_c measure(ImFont *font, float size, const char *text)
{
	return ImFont_CalcTextSizeA(font, size, FLT_MAX, 0.0f, text, NULL, NULL);
}

static void draw_text(ImFont *font, float size, float x, float y, ImU32 color, const char *text)
{
	ImDrawList_AddText_FontPtr(DL, font, size, v2(x, y), color, text, NULL, 0.0f, NULL);
}

static void draw_text_center(ImFont *font, float size, float cx, float cy, ImU32 color, const char *text)
{
	ImVec2_c size_px = measure(font, size, text);
	draw_text(font, size, cx - size_px.x * 0.5f, cy - size * 0.5f, color, text);
}

static void draw_text_right(ImFont *font, float size, float right, float cy, ImU32 color, const char *text)
{
	ImVec2_c size_px = measure(font, size, text);
	draw_text(font, size, right - size_px.x, cy - size * 0.5f, color, text);
}

static void draw_text_ellipsis(ImFont *font, float size, float x, float cy, float max_w, ImU32 color, const char *text)
{
	char buffer[1024];
	size_t len;
	size_t lo, hi;
	static const char dots[] = "\xE2\x80\xA6";

	if (!text || !text[0] || max_w <= 0.0f)
		return;
	len = strlen(text);
	if (len >= sizeof(buffer) - 4)
		len = sizeof(buffer) - 5;
	memcpy(buffer, text, len);
	buffer[len] = 0;
	if (measure(font, size, buffer).x <= max_w) {
		draw_text(font, size, x, cy - size * 0.5f, color, buffer);
		return;
	}
	lo = 0;
	hi = len;
	while (lo < hi) {
		size_t mid = (lo + hi + 1) / 2;
		float width;
		buffer[mid] = 0;
		width = measure(font, size, buffer).x + measure(font, size, dots).x;
		buffer[mid] = text[mid];
		if (width <= max_w)
			lo = mid;
		else
			hi = mid - 1;
	}
	while (lo > 0 && ((unsigned char)text[lo] & 0xC0) == 0x80)
		lo--;
	memcpy(buffer, text, lo);
	memcpy(buffer + lo, dots, sizeof(dots));
	draw_text(font, size, x, cy - size * 0.5f, color, buffer);
}

static void fill_round(float x0, float y0, float x1, float y1, float rounding, ImU32 color)
{
	ImDrawList_AddRectFilled(DL, v2(x0, y0), v2(x1, y1), color, rounding, 0);
}

static void stroke_round(float x0, float y0, float x1, float y1, float rounding, float thickness, ImU32 color)
{
	ImDrawList_AddRect(DL, v2(x0, y0), v2(x1, y1), color, rounding, thickness, 0);
}

static void fill_rect(float x0, float y0, float x1, float y1, ImU32 color)
{
	ImDrawList_AddRectFilled(DL, v2(x0, y0), v2(x1, y1), color, 0.0f, 0);
}

static char *u8w(const wchar_t *text)
{
	static char buffers[8][4096];
	static int index;
	char *out;
	int need;
	index = (index + 1) % 8;
	out = buffers[index];
	if (!text) {
		out[0] = 0;
		return out;
	}
	need = WideCharToMultiByte(CP_UTF8, 0, text, -1, out, 4095, NULL, NULL);
	if (need <= 0)
		out[0] = 0;
	return out;
}

static char *u8upper_w(const wchar_t *text)
{
	static wchar_t buffer[4096];
	size_t len;
	if (!text) {
		buffer[0] = 0;
		return u8w(buffer);
	}
	len = wcslen(text);
	if (len > 4095)
		len = 4095;
	memcpy(buffer, text, len * sizeof(wchar_t));
	buffer[len] = 0;
	CharUpperBuffW(buffer, (DWORD)len);
	return u8w(buffer);
}

static float button_width(const char *label, int min_units)
{
	float min = (float)S(min_units);
	float needed = measure((ImFont *)A->font_ui, T, label).x + (float)S(28);
	return needed > min ? needed : min;
}

static void glyph_set(char out[5], wchar_t code)
{
	char temp[8];
	int len;
	len = WideCharToMultiByte(CP_UTF8, 0, &code, 1, temp, 7, NULL, NULL);
	if (len < 0)
		len = 0;
	temp[len] = 0;
	strncpy(out, temp, 4);
	out[4] = 0;
}

static const wchar_t *base_name_w(const wchar_t *path)
{
	const wchar_t *slash;
	const wchar_t *forward;
	if (!path)
		return L"";
	slash = wcsrchr(path, L'\\');
	forward = wcsrchr(path, L'/');
	if (forward && (!slash || forward > slash))
		slash = forward;
	return slash ? slash + 1 : path;
}

static void parent_folder_w(const wchar_t *path, wchar_t *out, size_t cap)
{
	const wchar_t *slash;
	size_t len;
	out[0] = 0;
	if (!path)
		return;
	slash = wcsrchr(path, L'\\');
	if (!slash)
		return;
	len = (size_t)(slash - path);
	if (len >= cap)
		len = cap - 1;
	memcpy(out, path, len * sizeof(wchar_t));
	out[len] = 0;
	{
		wchar_t *second = wcsrchr(out, L'\\');
		if (second) {
			wchar_t *cursor = out;
			wchar_t *prev = NULL;
			while (cursor < second) {
				if (*cursor == L'\\')
					prev = cursor;
				cursor++;
			}
			if (prev)
				memmove(out, prev, (wcslen(prev) + 1) * sizeof(wchar_t));
		}
	}
}

static void style_apply(App *app)
{
	ImGuiStyle *style = igGetStyle();
	ImU32 text_disabled = alpha_color(TH->muted, 150);

	style->WindowPadding = v2(0.0f, 0.0f);
	style->WindowRounding = 0.0f;
	style->WindowBorderSize = 0.0f;
	style->ChildRounding = 0.0f;
	style->ChildBorderSize = 0.0f;
	style->FramePadding = v2((float)S(8), (float)S(5));
	style->FrameRounding = (float)S(8);
	style->FrameBorderSize = 0.0f;
	style->ItemSpacing = v2((float)S(6), (float)S(6));
	style->ItemInnerSpacing = v2((float)S(5), (float)S(4));
	style->ScrollbarSize = (float)S(10);
	style->ScrollbarRounding = (float)S(6);
	style->GrabRounding = (float)S(6);
	style->PopupRounding = (float)S(8);
	style->PopupBorderSize = 1.0f;
	style->WindowMinSize = v2(120.0f, 60.0f);

	style->Colors[ImGuiCol_Text] = color4(TH->ink);
	style->Colors[ImGuiCol_TextDisabled] = color4(text_disabled);
	style->Colors[ImGuiCol_WindowBg] = color4(TH->surface);
	style->Colors[ImGuiCol_ChildBg] = color4(TH->surface);
	style->Colors[ImGuiCol_PopupBg] = color4(TH->tip_bg);
	style->Colors[ImGuiCol_Border] = color4(TH->border);
	style->Colors[ImGuiCol_BorderShadow] = color4(0);
	style->Colors[ImGuiCol_FrameBg] = color4(TH->field);
	style->Colors[ImGuiCol_FrameBgHovered] = color4(TH->hover);
	style->Colors[ImGuiCol_FrameBgActive] = color4(TH->active);
	style->Colors[ImGuiCol_TitleBg] = color4(TH->chrome);
	style->Colors[ImGuiCol_TitleBgActive] = color4(TH->chrome);
	style->Colors[ImGuiCol_TitleBgCollapsed] = color4(TH->chrome);
	style->Colors[ImGuiCol_ScrollbarBg] = color4(0);
	style->Colors[ImGuiCol_ScrollbarGrab] = color4(alpha_color(TH->muted, 120));
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = color4(alpha_color(TH->muted, 190));
	style->Colors[ImGuiCol_ScrollbarGrabActive] = color4(TH->accent);
	style->Colors[ImGuiCol_CheckMark] = color4(TH->accent);
	style->Colors[ImGuiCol_SliderGrab] = color4(TH->accent);
	style->Colors[ImGuiCol_SliderGrabActive] = color4(TH->accent_hover);
	style->Colors[ImGuiCol_Button] = color4(TH->surface);
	style->Colors[ImGuiCol_ButtonHovered] = color4(TH->hover);
	style->Colors[ImGuiCol_ButtonActive] = color4(TH->active);
	style->Colors[ImGuiCol_Header] = color4(TH->accent_soft);
	style->Colors[ImGuiCol_HeaderHovered] = color4(TH->hover);
	style->Colors[ImGuiCol_HeaderActive] = color4(TH->active);
	style->Colors[ImGuiCol_Separator] = color4(TH->border_soft);
	style->Colors[ImGuiCol_SeparatorHovered] = color4(TH->accent);
	style->Colors[ImGuiCol_SeparatorActive] = color4(TH->accent_hover);
	style->Colors[ImGuiCol_TextSelectedBg] = color4(alpha_color(TH->accent, 90));
	style->Colors[ImGuiCol_ModalWindowDimBg] = color4(TH->dim);
	style->Colors[ImGuiCol_NavCursor] = color4(TH->accent);

	style->_NextFrameFontSizeBase = T;
	style->FontSizeBase = T;
	style->FontScaleMain = 1.0f;
	style->FontScaleDpi = 1.0f;

	g_styled_dark = app->state.dark;
	g_styled_ui = U;
	g_styled_text = app->text_size;
}

static void tooltip_show(const char *text)
{
	ImGuiIO *io = igGetIO_Nil();
	(void)io;
	igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, v2((float)S(10), (float)S(6)));
	igPushStyleColor_U32(ImGuiCol_PopupBg, TH->tip_bg);
	igPushStyleColor_U32(ImGuiCol_Text, TH->tip_fg);
	igPushStyleColor_U32(ImGuiCol_Border, alpha_color(TH->tip_fg, 28));
	if (igBeginTooltip()) {
		igPushFont((ImFont *)A->font_small, T);
		igTextUnformatted(text, NULL);
		igPopFont();
		igEndTooltip();
	}
	igPopStyleColor(3);
	igPopStyleVar(1);
}

static int tool_button(const char *id, const char *glyph, float x, float y, float size,
	float icon_size, const char *tip, int enabled, int active, int primary)
{
	ImGuiIO *io = igGetIO_Nil();
	ImU32 background = 0;
	ImU32 icon_color;
	int clicked = 0;
	int hovered = 0;

	if (!enabled) {
		draw_text_center((ImFont *)A->font_icon, icon_size, x + size * 0.5f,
			y + size * 0.5f, alpha_color(TH->muted, 110), glyph);
		return 0;
	}

	igSetCursorScreenPos(v2(x, y));
	clicked = igInvisibleButton(id, v2(size, size), ImGuiButtonFlags_MouseButtonLeft) ? 1 : 0;
	hovered = igIsItemHovered(ImGuiHoveredFlags_None) ? 1 : 0;
	if (igIsItemActive())
		hovered = 1;
	if (hovered && tip)
		igIsItemHovered(ImGuiHoveredFlags_DelayNormal) ? tooltip_show(tip) : (void)0;

	if (primary) {
		background = hovered ? TH->accent_hover : TH->accent;
		icon_color = TH->on_accent;
	} else {
		if (active)
			background = TH->accent_soft;
		if (hovered)
			background = io->MouseDown[0] ? TH->active : TH->hover;
		icon_color = active ? TH->accent : TH->ink;
	}
	if (background)
		fill_round(x, y, x + size, y + size, (float)S(8), background);
	draw_text_center((ImFont *)A->font_icon, icon_size, x + size * 0.5f,
		y + size * 0.5f, icon_color, glyph);
	return clicked;
}

static void draw_mark(float x, float y, float size)
{
	float radius = size * 0.26f;
	fill_round(x, y, x + size, y + size, radius, TH->accent);
	draw_text_center((ImFont *)A->font_icon, size * 0.56f, x + size * 0.5f,
		y + size * 0.5f, TH->on_accent, G.doc);
}

static void draw_ai_mark(float x, float y, float size)
{
	float radius = size * 0.26f;
	fill_round(x, y, x + size, y + size, radius, TH->accent);
	draw_text_center((ImFont *)A->font_icon, size * 0.56f, x + size * 0.5f,
		y + size * 0.5f, TH->on_accent, G.sparkle);
}

static int text_button(const char *id, const char *label, float x, float y, float w,
	float h, int primary)
{
	ImGuiIO *io = igGetIO_Nil();
	int clicked;
	int hovered;
	ImU32 background;
	ImU32 color;
	int pressed;

	igSetCursorScreenPos(v2(x, y));
	clicked = igInvisibleButton(id, v2(w, h), ImGuiButtonFlags_MouseButtonLeft) ? 1 : 0;
	hovered = igIsItemHovered(ImGuiHoveredFlags_None) ? 1 : 0;
	pressed = hovered && io->MouseDown[0];
	if (primary) {
		background = pressed ? TH->accent_hover : (hovered ? TH->accent_hover : TH->accent);
		color = TH->on_accent;
	} else {
		background = pressed ? TH->active : (hovered ? TH->hover : 0);
		color = TH->ink;
	}
	if (background)
		fill_round(x, y, x + w, y + h, (float)S(9), background);
	if (!primary)
		stroke_round(x + 0.5f, y + 0.5f, x + w - 0.5f, y + h - 0.5f, (float)S(9),
			1.0f, TH->border);
	draw_text_center((ImFont *)A->font_ui, T, x + w * 0.5f,
		y + h * 0.5f, color, label);
	return clicked;
}

static int link_button(const char *id, const char *label, float x, float y, float w,
	float h)
{
	int clicked;
	int hovered;

	igSetCursorScreenPos(v2(x, y));
	clicked = igInvisibleButton(id, v2(w, h), ImGuiButtonFlags_MouseButtonLeft) ? 1 : 0;
	hovered = igIsItemHovered(ImGuiHoveredFlags_None) ? 1 : 0;
	if (hovered)
		fill_round(x, y, x + w, y + h, (float)S(9), TH->hover);
	stroke_round(x + 0.5f, y + 0.5f, x + w - 0.5f, y + h - 0.5f, (float)S(9),
		1.0f, hovered ? TH->accent : TH->border);
	draw_text_center((ImFont *)A->font_ui, T, x + w * 0.5f,
		y + h * 0.5f, hovered ? TH->accent : TH->ink, label);
	return clicked;
}

static void open_url(const wchar_t *url)
{
	ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
}

static void draw_spinner(float cx, float cy, float radius, ImU32 color)
{
	double time = igGetTime();
	int i;
	int segments = 22;
	double start = time * 3.4;
	for (i = 0; i < segments; i++) {
		double a0 = start + (double)i * 6.2831853 / (double)segments;
		double a1 = a0 + 6.2831853 / (double)segments * 1.15;
		ImU32 shade = alpha_color(color, (int)(40.0 + 200.0 * (double)i / (double)segments));
		ImDrawList_AddLine(DL, v2(cx + (float)cos(a0) * radius, cy + (float)sin(a0) * radius),
			v2(cx + (float)cos(a1) * radius, cy + (float)sin(a1) * radius), shade,
			(float)S(2) * 0.5f, 0);
	}
}

static void theme_toggle(void)
{
	A->state.dark = !A->state.dark;
	app_save_state(A);
	app_set_status(A, 0, L"%s", A->state.dark ? L"Modo escuro ativado" : L"Modo claro ativado");
}

static void find_open_now(void)
{
	A->find_open = 1;
	A->focus_find = 1;
	g_jump_guard = 0;
	if (A->query[0]) {
		A->search_pending = 1;
		A->search_pending_at = igGetTime() + 0.02;
	}
}

static void find_apply_now(void)
{
	g_jump_guard = 0;
	app_find_restart(A);
}

static void draw_tabs(float width, int top, int tab_h)
{
	float left = (float)S(14);
	float right = width - (float)S(14);
	float gap = (float)S(3);
	float plus_w = (float)S(30);
	float tab_w;
	float avail;
	float ty = (float)top + (float)S(4);
	float th = (float)tab_h - (float)S(4);
	int switch_to = -1;
	int close_index = -1;
	int new_tab = 0;
	int i;

	fill_rect(0, (float)top, width, (float)(top + tab_h), TH->chrome);
	fill_rect(0, (float)(top + tab_h) - 1.0f, width, (float)(top + tab_h), TH->border);
	if (A->tab_count <= 0)
		return;

	avail = right - left - plus_w - gap;
	tab_w = avail / (float)A->tab_count - gap;
	if (tab_w > (float)S(210))
		tab_w = (float)S(210);
	if (tab_w < (float)S(64))
		tab_w = (float)S(64);

	for (i = 0; i < A->tab_count; i++) {
		Tab *tab = &A->tabs[i];
		int active = i == A->active_tab;
		const wchar_t *path = active ? A->doc.path : tab->doc.path;
		float tx = left + (float)i * (tab_w + gap);
		float close_w = tab_w >= (float)S(78) ? (float)S(22) : 0.0f;
		float body_w = tab_w - close_w;
		char id[40];
		ImU32 background = 0;
		ImU32 text_color;
		ImU32 icon_color;

		snprintf(id, sizeof(id), "##tab%d", tab->id);
		igSetCursorScreenPos(v2(tx, ty));
		igInvisibleButton(id, v2(body_w > 1.0f ? body_w : 1.0f, th),
			ImGuiButtonFlags_MouseButtonLeft);
		{
			int hovered = igIsItemHovered(ImGuiHoveredFlags_None) ? 1 : 0;
			if (igIsItemClicked(ImGuiMouseButton_Left))
				switch_to = i;
			if (igIsItemClicked(ImGuiMouseButton_Middle))
				close_index = i;
			if (igIsItemHovered(ImGuiHoveredFlags_DelayNormal))
				tooltip_show(u8w(path ? path : L""));
			if (active)
				background = TH->surface;
			else if (hovered)
				background = TH->hover;
		}
		if (background)
			fill_round(tx, ty, tx + tab_w, ty + th, (float)S(8), background);
		if (active)
			fill_rect(tx + (float)S(8), ty + th - 2.0f, tx + tab_w - (float)S(8),
				ty + th, TH->accent);
		text_color = active ? TH->ink : TH->muted;
		icon_color = active ? TH->accent : TH->muted;
		draw_text_center((ImFont *)A->font_icon_small, T * 0.82f,
			tx + (float)S(13), ty + th * 0.5f, icon_color, G.doc);
		{
			float text_x = tx + (float)S(26);
			float text_w = tab_w - (float)S(26) - close_w - (float)S(6);
			if (text_w < (float)S(8))
				text_w = (float)S(8);
			draw_text_ellipsis((ImFont *)A->font_ui, T, text_x,
				ty + th * 0.5f, text_w, text_color, u8upper_w(base_name_w(path)));
		}
		if (close_w > 0.0f) {
			char close_id[48];
			float cx = tx + tab_w - close_w;
			float cy = ty + th * 0.5f;
			int hovered;
			snprintf(close_id, sizeof(close_id), "##tabclose%d", tab->id);
			igSetCursorScreenPos(v2(cx, cy - (float)S(10)));
			igInvisibleButton(close_id, v2((float)S(20), (float)S(20)),
				ImGuiButtonFlags_MouseButtonLeft);
			hovered = igIsItemHovered(ImGuiHoveredFlags_None) ? 1 : 0;
			if (hovered)
				fill_round(cx, cy - (float)S(10), cx + (float)S(20),
					cy + (float)S(10), (float)S(5), TH->hover);
			draw_text_center((ImFont *)A->font_icon_small, T * 0.53f,
				cx + (float)S(10), cy,
				hovered ? TH->ink : alpha_color(TH->muted, 190), G.close_small);
			if (igIsItemClicked(ImGuiMouseButton_Left))
				close_index = i;
			if (hovered)
				igIsItemHovered(ImGuiHoveredFlags_DelayNormal) ?
					tooltip_show("Fechar documento (Ctrl+W)") : (void)0;
		}
	}
	{
		float px = left + (float)A->tab_count * (tab_w + gap) + (float)S(4);
		if (px + plus_w <= width - (float)S(8)) {
			if (tool_button("##tab_new", G.plus, px,
				ty + (th - (float)S(30)) * 0.5f, plus_w, T * 0.82f,
				"Abrir em nova aba (Ctrl+T)", 1, 0, 0))
				new_tab = 1;
		}
	}
	if (switch_to >= 0)
		app_switch_tab(A, switch_to);
	if (close_index >= 0)
		app_close_tab(A, close_index);
	if (new_tab)
		app_open_dialog(A);
}

static void draw_titlebar(float width, int title_h)
{
	ImGuiIO *io = igGetIO_Nil();
	float btn_w = (float)S(46);
	float close_x = width - btn_w;
	float max_x = close_x - btn_w;
	float min_x = max_x - btn_w;
	float drag_w = min_x - (float)S(8);
	float mark = (float)S(22);
	float mark_x = (float)S(14);
	float mark_y = ((float)title_h - mark) * 0.5f;
	float text_x = mark_x + mark + (float)S(10);
	float title_size = T;
	float cy = (float)title_h * 0.5f;

	fill_rect(0, 0, width, (float)title_h, TH->chrome);
	fill_rect(0, (float)title_h - 1.0f, width, (float)title_h, TH->border);

	igSetCursorScreenPos(v2((float)S(14), 0));
	igInvisibleButton("##title_drag", v2(drag_w > 1.0f ? drag_w : 1.0f, (float)title_h),
		ImGuiButtonFlags_MouseButtonLeft);
	if (igIsItemClicked(ImGuiMouseButton_Left)) {
		ReleaseCapture();
		if (io->MouseDoubleClicked[0])
			SendMessageW(A->hwnd, WM_NCLBUTTONDBLCLK, HTCAPTION, 0);
		else
			SendMessageW(A->hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
	}

	draw_mark(mark_x, mark_y, mark);
	draw_text((ImFont *)A->font_bold, title_size, text_x, cy - title_size * 0.5f,
		TH->ink, "PDF LEVE");
	text_x += measure((ImFont *)A->font_bold, title_size, "PDF LEVE").x + (float)S(12);

	if (A->doc.doc && A->doc.path) {
		draw_text_center((ImFont *)A->font_ui, T, text_x - (float)S(4),
			cy, TH->muted, "\xC2\xB7");
		text_x += (float)S(8);
		draw_text_ellipsis((ImFont *)A->font_ui, T, text_x, cy,
			min_x - (float)S(8) - text_x, TH->muted, u8upper_w(base_name_w(A->doc.path)));
	} else {
		draw_text_ellipsis((ImFont *)A->font_ui, T, text_x, cy,
			min_x - (float)S(8) - text_x, TH->muted,
			"LEITOR DE PDF  \xC2\xB7  POR DANIEL ORTEGA PEREIRA");
	}

	{
		float xs[3];
		const char *icons[3];
		int i;
		xs[0] = min_x;
		xs[1] = max_x;
		xs[2] = close_x;
		icons[0] = G.min;
		icons[1] = IsZoomed(A->hwnd) ? G.restore : G.max;
		icons[2] = G.close;
		for (i = 0; i < 3; i++) {
			float bx = xs[i];
			UiRect br = rectf(bx, 0.0f, bx + btn_w, (float)title_h);
			int hovered;
			int pressed;
			igSetCursorScreenPos(v2(br.x0, br.y0));
			igInvisibleButton(i == 0 ? "##win_min" : (i == 1 ? "##win_max" : "##win_close"),
				v2(btn_w, (float)title_h), ImGuiButtonFlags_MouseButtonLeft);
			hovered = igIsItemHovered(ImGuiHoveredFlags_None) ? 1 : 0;
			pressed = hovered && io->MouseDown[0];
			if (hovered) {
				ImU32 bg = i == 2 ? TH->close_hover : TH->hover;
				if (pressed)
					bg = i == 2 ? IM_COL32(0xA8, 0x34, 0x28, 255) : TH->active;
				fill_rect(br.x0, br.y0, br.x1, br.y1, bg);
			}
			draw_text_center((ImFont *)A->font_icon, T * 0.62f,
				(br.x0 + br.x1) * 0.5f, (br.y0 + br.y1) * 0.5f,
				(i == 2 && hovered) ? IM_COL32(255, 255, 255, 255) : TH->ink, icons[i]);
			if (igIsItemClicked(ImGuiMouseButton_Left)) {
				if (i == 0)
					ShowWindow(A->hwnd, SW_MINIMIZE);
				else if (i == 1) {
					if (IsZoomed(A->hwnd))
						ShowWindow(A->hwnd, SW_RESTORE);
					else
						ShowWindow(A->hwnd, SW_MAXIMIZE);
				} else {
					PostMessageW(A->hwnd, WM_CLOSE, 0, 0);
				}
			}
		}
	}
}

static void draw_toolbar(float width, int top, int toolbar_h)
{
	float y = (float)top + (float)(toolbar_h - S(38)) / 2.0f;
	float x = (float)S(14);
	float right = width - (float)S(14);
	float ic = T;
	float page_h = (float)S(30);
	float page_w = (float)S(82);
	float page_y = (float)top + ((float)toolbar_h - page_h) * 0.5f;
	float page_x;
	float sep_x;
	float theme_x;
	float info_x;
	float ocr_x;
	float ai_x;
	float copy_x;
	float mark_x;
	float search_x;
	float rotate_x;
	float fit_x;
	float zin_x;
	float pill_x;
	float zout_x;
	float sb_x;
	float set_x;
	float exp_x;
	float pill_w = (float)S(54);
	float pill_h = (float)S(28);
	int has_doc = A->doc.doc != NULL;
	char label[128];

	if (has_doc) {
		snprintf(label, sizeof(label), "%d / %d", A->doc.page_count, A->doc.page_count);
		{
			float need = measure((ImFont *)A->font_bold, T, label).x + (float)S(24);
			if (need > page_w)
				page_w = need;
		}
	}
	{
		float need = measure((ImFont *)A->font_bold, T, "600%").x + (float)S(20);
		if (need > pill_w)
			pill_w = need;
	}
	if (T + (float)S(10) > pill_h)
		pill_h = T + (float)S(10);

	fill_rect(0, (float)top, width, (float)(top + toolbar_h), TH->chrome);
	fill_rect(0, (float)(top + toolbar_h) - 1.0f, width, (float)(top + toolbar_h), TH->border);

	if (tool_button("##tb_open", G.open, x, y, (float)S(38), ic + 1.0f,
		"Abrir PDF (Ctrl+O)", 1, 0, 1)) {
		app_open_dialog(A);
	}
	x += (float)S(38) + (float)S(10);
	{
		float by = y + 1.0f;
		if (tool_button("##tb_prev", G.prev, x, by, (float)S(36), ic,
			"Página anterior (PgUp)", has_doc && A->doc.page > 0, 0, 0))
			app_step_page(A, -1);
		x += (float)S(36) + (float)S(2);
		if (tool_button("##tb_next", G.next, x, by, (float)S(36), ic,
			"Próxima página (PgDn)", has_doc && A->doc.page + 1 < A->doc.page_count, 0, 0))
			app_step_page(A, 1);
		x += (float)S(36) + (float)S(10);
	}

	page_x = x;
	if (has_doc)
		snprintf(label, sizeof(label), "%d / %d", A->doc.page + 1, A->doc.page_count);
	else
		snprintf(label, sizeof(label), "— / —");
	fill_round(page_x, page_y, page_x + page_w, page_y + page_h,
		(float)S(8), TH->surface);
	stroke_round(page_x + 0.5f, page_y + 0.5f, page_x + page_w - 0.5f,
		page_y + page_h - 0.5f, (float)S(8), 1.0f, TH->border);
	draw_text_center((ImFont *)A->font_bold, T, page_x + page_w * 0.5f,
		page_y + page_h * 0.5f, has_doc ? TH->ink : TH->muted, label);

	info_x = right - (float)S(36);
	if (tool_button("##tb_about", G.info, info_x, y, (float)S(36), ic,
		"Sobre o PDF Leve (F1)", 1, g_about_open, 0))
		g_about_open = 1;
	theme_x = info_x - (float)S(6) - (float)S(36);
	if (tool_button("##tb_theme", A->state.dark ? G.sun : G.moon, theme_x, y,
		(float)S(36), ic, A->state.dark ? "Modo claro (Ctrl+Shift+D)" :
		"Modo escuro (Ctrl+Shift+D)", 1, 0, 0)) {
		theme_toggle();
	}
	ocr_x = theme_x - (float)S(10) - (float)S(36);
	ai_x = ocr_x - (float)S(4) - (float)S(36);
	copy_x = ai_x - (float)S(4) - (float)S(36);
	mark_x = copy_x - (float)S(4) - (float)S(36);
	search_x = mark_x - (float)S(4) - (float)S(36);
	sep_x = search_x - (float)S(8);
	rotate_x = sep_x - (float)S(8) - (float)S(36);
	fit_x = rotate_x - (float)S(4) - (float)S(36);
	zin_x = fit_x - (float)S(4) - (float)S(36);
	pill_x = zin_x - (float)S(4) - pill_w;
	zout_x = pill_x - (float)S(4) - (float)S(36);

	if (tool_button("##tb_zoom_out", G.zoom_out, zout_x, y, (float)S(36), ic,
		"Diminuir zoom (Ctrl+-)", has_doc && A->doc.zoom > MIN_ZOOM + 0.001f, 0, 0))
		app_zoom_by(A, 0.8f);

	fill_round(pill_x, y + (float)S(4), pill_x + pill_w, y + (float)S(4) + pill_h,
		(float)S(7), 0);
	{
		int hovered;
		igSetCursorScreenPos(v2(pill_x, y + (float)S(4)));
		igInvisibleButton("##tb_zoom_pill", v2(pill_w, pill_h), ImGuiButtonFlags_MouseButtonLeft);
		hovered = igIsItemHovered(ImGuiHoveredFlags_None) ? 1 : 0;
		if (hovered) {
			igIsItemHovered(ImGuiHoveredFlags_DelayNormal) ? tooltip_show("Voltar para 100%") : (void)0;
			fill_round(pill_x, y + (float)S(4), pill_x + pill_w, y + (float)S(4) + pill_h,
				(float)S(7), TH->hover);
		}
		if (has_doc)
			snprintf(label, sizeof(label), "%d%%", (int)lroundf(A->doc.zoom * 100.0f));
		else
			snprintf(label, sizeof(label), "—");
		draw_text_center((ImFont *)A->font_bold, T, pill_x + pill_w * 0.5f,
			y + (float)S(4) + pill_h * 0.5f, has_doc ? (hovered ? TH->ink : TH->muted) :
			alpha_color(TH->muted, 120), label);
		if (igIsItemClicked(ImGuiMouseButton_Left) && has_doc)
			app_set_zoom(A, 1.0f);
	}

	sb_x = page_x + page_w + (float)S(10);
	if (tool_button("##tb_sidebar", G.sidebar, sb_x, y, (float)S(36), ic,
		"Páginas e sumário (Ctrl+B)", has_doc, has_doc && A->sidebar_open, 0))
		app_toggle_sidebar(A);
	set_x = sb_x + (float)S(36) + (float)S(4);
	g_settings_btn = rectf(set_x, y, set_x + (float)S(36), y + (float)S(36));
	if (tool_button("##tb_settings", G.settings, set_x, y, (float)S(36), ic,
		"Ajustes da interface", 1, g_settings_open, 0)) {
		g_settings_open = !g_settings_open;
		g_settings_age = 1;
	}
	if (width >= (float)S(1040)) {
		exp_x = set_x + (float)S(36) + (float)S(4);
		if (tool_button("##tb_export", G.export_txt, exp_x, y, (float)S(36), ic,
			"Exportar texto da página (.txt)", has_doc, 0, 0))
			app_export_page_text(A);
	}

	if (tool_button("##tb_zoom_in", G.zoom_in, zin_x, y, (float)S(36), ic,
		"Aumentar zoom (Ctrl++)", has_doc && A->doc.zoom < MAX_ZOOM - 0.001f, 0, 0))
		app_zoom_by(A, 1.25f);

	{
		const char *fit_tip = "Ajustar à largura";
		if (has_doc && A->doc.fit == FIT_WIDTH)
			fit_tip = "Ajustar à página";
		else if (has_doc && A->doc.fit == FIT_PAGE)
			fit_tip = "Tamanho real (100%)";
		if (tool_button("##tb_fit", G.fit, fit_x, y, (float)S(36), ic, fit_tip,
			has_doc, has_doc && A->doc.fit != FIT_NONE, 0)) {
			if (A->doc.fit == FIT_WIDTH)
				app_fit(A, FIT_PAGE);
			else if (A->doc.fit == FIT_PAGE)
				app_set_zoom(A, 1.0f);
			else
				app_fit(A, FIT_WIDTH);
		}
	}
	if (tool_button("##tb_rotate", G.rotate, rotate_x, y, (float)S(36), ic,
		"Girar 90°", has_doc, 0, 0))
		app_rotate(A);

	fill_rect(sep_x, (float)top + (float)S(16), sep_x + 1.0f,
		(float)(top + toolbar_h) - (float)S(16), TH->border);

	{
		int search_active = A->find_open;
		if (tool_button("##tb_search", G.search, search_x, y, (float)S(36), ic,
			"Localizar (Ctrl+F)", has_doc, search_active, 0)) {
			if (A->find_open) {
				app_find_close(A);
				A->focus_find = 0;
				snprintf(g_last_query, sizeof(g_last_query), "%s", A->query);
			} else {
				find_open_now();
			}
		}
	}
	if (tool_button("##tb_mark", G.mark, mark_x, y, (float)S(36), ic,
		"Marcar ou desmarcar trecho (Ctrl+H)", has_doc,
		has_doc && app_has_mark_on_page(A, A->doc.page), 0))
		app_toggle_mark(A);
	if (tool_button("##tb_copy", G.copy, copy_x, y, (float)S(36), ic,
		"Copiar texto da página", has_doc, 0, 0))
		app_copy(A);
	if (A->ocr_busy) {
		igSetCursorScreenPos(v2(ocr_x, y));
		igInvisibleButton("##tb_ocr", v2((float)S(36), (float)S(36)), ImGuiButtonFlags_MouseButtonLeft);
		draw_spinner(ocr_x + (float)S(18), y + (float)S(18), 7.0f * U, TH->accent);
		if (igIsItemHovered(ImGuiHoveredFlags_DelayNormal))
			tooltip_show("Reconhecendo texto…");
	} else {
		if (tool_button("##tb_ocr", G.ocr, ocr_x, y, (float)S(36), ic,
			"Reconhecer texto (OCR)", has_doc, 0, 0))
			app_start_ocr(A);
	}
	if (A->ai_busy) {
		igSetCursorScreenPos(v2(ai_x, y));
		igInvisibleButton("##tb_ai", v2((float)S(36), (float)S(36)), ImGuiButtonFlags_MouseButtonLeft);
		draw_spinner(ai_x + (float)S(18), y + (float)S(18), 7.0f * U, TH->accent);
		if (igIsItemHovered(ImGuiHoveredFlags_DelayNormal))
			tooltip_show("Consultando o Gemini…");
	} else {
		if (tool_button("##tb_ai", G.sparkle, ai_x, y, (float)S(36), ic,
			"Gemini: resumir ou perguntar (Ctrl+G)", 1, A->ai_open, 0))
			app_ai_open_modal(A);
	}
}

static void draw_findbar(float width, int top, int find_h)
{
	ImGuiIO *io = igGetIO_Nil();
	float field_h = T + (float)S(16);
	float field_y = (float)top + ((float)find_h - field_h) * 0.5f;
	float close_x = width - (float)S(14) - (float)S(30);
	float next_x = close_x - (float)S(4) - (float)S(30);
	float prev_x = next_x - (float)S(4) - (float)S(30);
	float field_x = (float)S(14);
	float reserve = measure((ImFont *)A->font_ui, T, "9999 de 9999").x +
		(float)S(12);
	float field_w;
	float count_right = prev_x - (float)S(6);
	float pad_y;
	char info[160];
	int focused;

	if (field_h < (float)S(30))
		field_h = (float)S(30);
	if (reserve < (float)S(88))
		reserve = (float)S(88);
	field_w = prev_x - reserve - field_x;
	if (field_w < (float)S(120))
		field_w = (float)S(120);
	pad_y = (field_h - T) * 0.5f - 1.0f;
	if (pad_y < 2.0f)
		pad_y = 2.0f;

	fill_rect(0, (float)top, width, (float)(top + find_h), TH->chrome);
	fill_rect(0, (float)(top + find_h) - 1.0f, width, (float)(top + find_h), TH->border);

	fill_round(field_x, field_y, field_x + field_w, field_y + field_h, (float)S(8), TH->surface);
	draw_text_center((ImFont *)A->font_icon_small, T * 0.76f, field_x + (float)S(18),
		field_y + field_h * 0.5f, TH->muted, G.search);

	igSetCursorScreenPos(v2(field_x + (float)S(32), field_y));
	igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, v2((float)S(4), pad_y));
	igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 0.0f);
	igPushStyleColor_U32(ImGuiCol_FrameBg, 0);
	igPushStyleColor_U32(ImGuiCol_FrameBgHovered, 0);
	igPushStyleColor_U32(ImGuiCol_FrameBgActive, 0);
	igSetNextItemWidth(field_w - (float)S(38));
	if (A->focus_find) {
		igSetKeyboardFocusHere(0);
		A->focus_find = 0;
	}
	{
		int enter = igInputTextWithHint("##find", "Localizar no documento", A->query,
			sizeof(A->query), ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL) ? 1 : 0;
		focused = igIsItemActive() ? 1 : 0;
		if (strcmp(A->query, g_last_query) != 0) {
			snprintf(g_last_query, sizeof(g_last_query), "%s", A->query);
			A->search_pending = A->query[0] != 0;
			A->search_pending_at = igGetTime() + 0.30;
			g_jump_guard = 0;
		}
		if (enter) {
			if (A->query[0] && strcmp(A->query, A->query_applied) != 0)
				find_apply_now();
			else
				app_find_step(A, io->KeyShift ? -1 : 1);
			A->search_pending = 0;
		}
	}
	igPopStyleColor(3);
	igPopStyleVar(2);
	stroke_round(field_x + 0.5f, field_y + 0.5f, field_x + field_w - 0.5f,
		field_y + field_h - 0.5f, (float)S(8), 1.0f,
		focused ? TH->accent : (A->find_open ? TH->border : TH->border));

	if (A->query[0]) {
		if (A->searching) {
			snprintf(info, sizeof(info), "%s", "Buscando\xE2\x80\xA6");
		} else if (A->hit_count == 0) {
			snprintf(info, sizeof(info), "Sem resultados");
		} else if (A->hit_index >= 0) {
			snprintf(info, sizeof(info), "%d de %d", A->hit_index + 1, A->hit_count);
		} else {
			snprintf(info, sizeof(info), "%d ocorr\xC3\xAAncias", A->hit_count);
		}
		draw_text_right((ImFont *)A->font_small, T, count_right,
			(float)top + (float)find_h * 0.5f, TH->muted, info);
	}

	if (tool_button("##find_prev", G.prev, prev_x, field_y, (float)S(30), T * 0.76f,
		"Ocorrência anterior (Shift+F3)", A->hit_count > 0, 0, 0))
		app_find_step(A, -1);
	if (tool_button("##find_next", G.next, next_x, field_y, (float)S(30), T * 0.76f,
		"Próxima ocorrência (F3)", A->hit_count > 0, 0, 0))
		app_find_step(A, 1);
	if (tool_button("##find_close", G.close_small, close_x, field_y, (float)S(30), T * 0.76f,
		"Fechar busca (Esc)", 1, 0, 0)) {
		app_find_close(A);
		snprintf(g_last_query, sizeof(g_last_query), "%s", A->query);
	}
}

static void draw_empty_state(void)
{
	UiRect view = g_view;
	float cx = (view.x0 + view.x1) * 0.5f;
	float content_h = (float)(S(84) + S(18) + S(34) + S(4) + S(20) + S(24) + S(54) +
		S(14) + S(18) + S(20));
	float available = (view.y1 - view.y0) - content_h - (float)S(24) - (float)S(34);
	float row_h = (float)S(50);
	float panel_w = (float)S(600);
	int rows = A->state.recent_count;
	int i;
	float start_y;
	float icon = (float)S(84);

	if (row_h < T * 2.0f + (float)S(10))
		row_h = T * 2.0f + (float)S(10);
	if (panel_w > (view.x1 - view.x0) - (float)S(80))
		panel_w = (view.x1 - view.x0) - (float)S(80);
	if (panel_w < (float)S(280))
		panel_w = (float)S(280);
	if (rows > 5)
		rows = 5;
	if (available < row_h)
		rows = 0;
	else if ((float)rows * row_h > available)
		rows = (int)(available / row_h);

	{
		float total = content_h + (rows > 0 ? (float)S(24) + (float)S(24) + rows * row_h : 0.0f);
		start_y = view.y0 + ((view.y1 - view.y0) - total) * 0.5f;
		if (start_y < view.y0 + (float)S(12))
			start_y = view.y0 + (float)S(12);
	}

	draw_mark(cx - icon * 0.5f, start_y, icon);
	draw_text_center((ImFont *)A->font_bold, T, cx, start_y + icon +
		(float)S(18) + (float)S(17), TH->ink, "PDF LEVE");
	draw_text_center((ImFont *)A->font_ui, T, cx,
		start_y + icon + (float)S(18) + (float)S(34) + (float)S(4) + (float)S(10),
		TH->muted, "Leve, rápido e sem distrações.");

	{
		const char *credit = "Desenvolvido por Daniel Ortega Pereira";
		ImFont *small = (ImFont *)A->font_small;
		float credit_size = T;
		float credit_cy = start_y + icon + (float)S(18) + (float)S(34) +
			(float)S(4) + (float)S(10) + (float)S(26);
		ImVec2_c credit_w = measure(small, credit_size, credit);
		int hovered;
		int clicked;

		igSetCursorScreenPos(v2(cx - credit_w.x * 0.5f - (float)S(6),
			credit_cy - credit_size * 0.5f - (float)S(3)));
		clicked = igInvisibleButton("##empty_about",
			v2(credit_w.x + (float)S(12), credit_size + (float)S(6)),
			ImGuiButtonFlags_MouseButtonLeft) ? 1 : 0;
		hovered = igIsItemHovered(ImGuiHoveredFlags_None) ? 1 : 0;
		draw_text_center(small, credit_size, cx, credit_cy,
			hovered ? TH->accent : alpha_color(TH->muted, 215), credit);
		if (hovered)
			igIsItemHovered(ImGuiHoveredFlags_DelayNormal) ? tooltip_show("Sobre o PDF Leve") : (void)0;
		if (clicked)
			g_about_open = 1;
	}

	{
		float by = start_y + content_h - (float)S(54) - (float)S(14) - (float)S(18);
		float bx = cx - (float)S(27);
		ImGuiIO *io = igGetIO_Nil();
		int hovered;
		int clicked;
		igSetCursorScreenPos(v2(bx, by));
		clicked = igInvisibleButton("##empty_open", v2((float)S(54), (float)S(54)),
			ImGuiButtonFlags_MouseButtonLeft) ? 1 : 0;
		hovered = igIsItemHovered(ImGuiHoveredFlags_None) ? 1 : 0;
		fill_round(bx, by, bx + (float)S(54), by + (float)S(54), (float)S(16),
			(hovered && io->MouseDown[0]) ? TH->accent_hover : TH->accent);
		draw_text_center((ImFont *)A->font_icon_big, T * 1.53f, cx,
			by + (float)S(27), TH->on_accent, G.open);
		if (hovered && igIsItemHovered(ImGuiHoveredFlags_DelayShort))
			tooltip_show("Abrir PDF (Ctrl+O)");
		if (clicked)
			app_open_dialog(A);
		draw_text_center((ImFont *)A->font_small, T, cx,
			by + (float)S(54) + (float)S(9), TH->muted,
			"Arraste um PDF aqui ou pressione Ctrl+O");
	}

	if (rows > 0) {
		float panel_top = start_y + content_h + (float)S(24);
		float panel_x0 = cx - panel_w * 0.5f;
		float panel_x1 = cx + panel_w * 0.5f;
		float panel_y0 = panel_top - (float)S(8);
		float panel_y1 = panel_top + rows * row_h + (float)S(8);
		draw_text((ImFont *)A->font_small, T, panel_x0 + (float)S(14),
			panel_y0 - T - (float)S(4), TH->muted, "RECENTES");
		fill_round(panel_x0, panel_y0, panel_x1, panel_y1, (float)S(12), TH->surface);
		stroke_round(panel_x0 + 0.5f, panel_y0 + 0.5f, panel_x1 - 0.5f, panel_y1 - 0.5f,
			(float)S(12), 1.0f, TH->border);

		for (i = 0; i < rows; i++) {
			Recent *recent = &A->state.recents[i];
			float rt = panel_top + i * row_h;
			UiRect row = rectf(panel_x0, rt, panel_x1, rt + row_h - (float)S(2));
			ImVec2_c mouse = igGetIO_Nil()->MousePos;
			int hovered;
			wchar_t name[512];
			wchar_t folder[512];
			wchar_t when[64];
			wchar_t sub[700];
			char id[32];
			float icon_sz = (float)S(32);
			float icon_x = row.x0 + (float)S(12);
			float icon_y = row.y0 + (row.y1 - row.y0 - icon_sz) * 0.5f;
			float text_left = row.x0 + (float)S(56);
			float time_w = (float)S(84);
			float cy_name = row.y0 + T * 0.5f + (float)S(3);
			float cy_sub = cy_name + T + (float)S(2);
			float need_time = measure((ImFont *)A->font_ui, T, "00/00/0000").x +
				(float)S(10);
			if (need_time > time_w)
				time_w = need_time;

			hovered = in_rect(row, mouse);
			if (hovered)
				fill_round(row.x0 + (float)S(4), row.y0 + (float)S(2), row.x1 - (float)S(4),
					row.y1 - (float)S(2), (float)S(8), TH->hover);
			fill_round(icon_x, icon_y, icon_x + icon_sz, icon_y + icon_sz, (float)S(8),
				TH->accent_soft);
			draw_text_center((ImFont *)A->font_icon, T * 0.94f, icon_x + icon_sz * 0.5f,
				icon_y + icon_sz * 0.5f, TH->accent, G.doc);
			wcsncpy(name, base_name_w(recent->path), 511);
			name[511] = 0;
			parent_folder_w(recent->path, folder, 512);
			_snwprintf(sub, 699, L"%s  \u00B7  página %d",
				folder[0] ? folder : L"Documento", recent->page + 1);
			sub[699] = 0;
			state_format_time(recent->used, when, 64);
			draw_text_ellipsis((ImFont *)A->font_bold, T, text_left, cy_name,
				row.x1 - (float)S(18) - time_w - text_left, TH->ink, u8w(name));
			draw_text_ellipsis((ImFont *)A->font_small, T, text_left, cy_sub,
				row.x1 - (float)S(18) - time_w - text_left, TH->muted, u8w(sub));
			draw_text_right((ImFont *)A->font_small, T, row.x1 - (float)S(14),
				(row.y0 + row.y1) * 0.5f, alpha_color(TH->muted, 190), u8w(when));
			if (i + 1 < rows)
				fill_rect(row.x0 + (float)S(56), row.y1 - 1.0f, row.x1 - (float)S(16),
					row.y1, TH->border_soft);
			snprintf(id, sizeof(id), "##recent%d", i);
			igSetCursorScreenPos(v2(row.x0, row.y0));
			igInvisibleButton(id, v2(row.x1 - row.x0, row.y1 - row.y0),
				ImGuiButtonFlags_MouseButtonLeft);
			if (igIsItemClicked(ImGuiMouseButton_Left))
				app_open_path(A, recent->path);
		}
	}
}

static ImVec2_c page_to_screen(fz_point point)
{
	Doc *doc = &A->doc;
	fz_point rotated = point;
	ImVec2_c out;
	if (doc->rotation)
		rotated = fz_transform_point(point, fz_rotate((float)doc->rotation));
	out.x = (float)A->rc_page.left + (rotated.x - doc->bounds.x0) * doc->scale;
	out.y = (float)A->rc_page.top + (rotated.y - doc->bounds.y0) * doc->scale;
	return out;
}

static fz_point screen_to_page(ImVec2_c point)
{
	Doc *doc = &A->doc;
	fz_point rotated;
	fz_point out;
	if (doc->scale <= 0.0f) {
		out.x = 0;
		out.y = 0;
		return out;
	}
	rotated.x = (point.x - (float)A->rc_page.left) / doc->scale + doc->bounds.x0;
	rotated.y = (point.y - (float)A->rc_page.top) / doc->scale + doc->bounds.y0;
	out = rotated;
	if (doc->rotation)
		out = fz_transform_point(rotated, fz_rotate((float)-doc->rotation));
	return out;
}

static void add_quad(fz_quad quad, ImU32 color)
{
	ImVec2_c p0 = page_to_screen(quad.ul);
	ImVec2_c p1 = page_to_screen(quad.ur);
	ImVec2_c p2 = page_to_screen(quad.lr);
	ImVec2_c p3 = page_to_screen(quad.ll);
	ImDrawList_AddQuadFilled(DL, p0, p1, p2, p3, color);
}

static void update_page_rect(UiRect view)
{
	Doc *doc = &A->doc;
	int margin = S(VIEW_MARGIN);
	float view_w = view.x1 - view.x0;
	float view_h = view.y1 - view.y0;
	float x, y;

	if (!doc->doc || doc->page_w <= 0 || doc->page_h <= 0) {
		SetRectEmpty(&A->rc_page);
		A->scroll_max = 0;
		A->scroll_max_x = 0;
		return;
	}
	A->scroll_max = doc->page_h + margin * 2 - (int)view_h;
	if (A->scroll_max < 0)
		A->scroll_max = 0;
	A->scroll_max_x = doc->page_w + margin * 2 - (int)view_w;
	if (A->scroll_max_x < 0)
		A->scroll_max_x = 0;

	if (A->scroll_y < 0)
		A->scroll_y = 0;
	if (A->scroll_y > A->scroll_max)
		A->scroll_y = A->scroll_max;
	if (A->scroll_x < 0)
		A->scroll_x = 0;
	if (A->scroll_x > A->scroll_max_x)
		A->scroll_x = A->scroll_max_x;

	if ((float)doc->page_w + (float)margin * 2.0f <= view_w)
		x = view.x0 + (view_w - (float)doc->page_w) * 0.5f;
	else
		x = view.x0 + (float)margin - (float)A->scroll_x;
	if ((float)doc->page_h + (float)margin * 2.0f <= view_h)
		y = view.y0 + (view_h - (float)doc->page_h) * 0.5f;
	else
		y = view.y0 + (float)margin - (float)A->scroll_y;

	A->rc_page.left = (LONG)lroundf(x);
	A->rc_page.top = (LONG)lroundf(y);
	A->rc_page.right = A->rc_page.left + doc->page_w;
	A->rc_page.bottom = A->rc_page.top + doc->page_h;
}

static int sidebar_thumb_slot(const App *app, int page)
{
	int i;
	for (i = 0; i < THUMB_SLOTS; i++) {
		if (app->thumbs[i].page == page)
			return i;
	}
	return -1;
}

static int sidebar_thumb_pick(App *app, int page, int first, int last)
{
	unsigned long long best = ~(unsigned long long)0;
	int pick = -1;
	int i;

	for (i = 0; i < THUMB_SLOTS; i++) {
		if (app->thumbs[i].page < 0)
			pick = i;
	}
	if (pick >= 0)
		goto done;
	for (i = 0; i < THUMB_SLOTS; i++) {
		int p = app->thumbs[i].page;
		if (p >= first && p <= last)
			continue;
		if (app->thumbs[i].used <= best) {
			best = app->thumbs[i].used;
			pick = i;
		}
	}
	if (pick < 0) {
		best = ~(unsigned long long)0;
		for (i = 0; i < THUMB_SLOTS; i++) {
			if (app->thumbs[i].used <= best) {
				best = app->thumbs[i].used;
				pick = i;
			}
		}
	}
done:
	app->thumbs[pick].page = page;
	app->thumbs[pick].w = 0;
	app->thumbs[pick].h = 0;
	app->thumbs[pick].used = ++app->thumb_clock;
	return pick;
}

static void sidebar_thumb_render(App *app, int page, int first, int last)
{
	unsigned char *pixels = NULL;
	int w = 0;
	int h = 0;
	int stride = 0;
	int slot = sidebar_thumb_pick(app, page, first, last);

	if (doc_render_thumb(&app->doc, page, S(136), S(154), &pixels, &w, &h, &stride)) {
		if (gfx_thumb_texture(slot, pixels, w, h, stride)) {
			app->thumbs[slot].w = w;
			app->thumbs[slot].h = h;
		}
	}
	free(pixels);
}

static void sidebar_scroll_indicator(UiRect content, float sw)
{
	float bar = (float)S(4);
	float track_y0 = content.y0 + (float)S(8);
	float track_h = (content.y1 - content.y0) - (float)S(16);
	float view_h = content.y1 - content.y0;
	float total = (float)view_h + (float)A->sb_scroll_max;
	float thumb_h;
	float y;

	if (A->sb_scroll_max <= 0 || track_h <= 0.0f)
		return;
	thumb_h = track_h * view_h / total;
	if (thumb_h < (float)S(26))
		thumb_h = (float)S(26);
	if (thumb_h > track_h)
		thumb_h = track_h;
	y = track_y0 + (track_h - thumb_h) *
		((float)A->sb_scroll / (float)A->sb_scroll_max);
	fill_round(sw - (float)S(8), track_y0, sw - (float)S(8) + bar,
		track_y0 + track_h, bar * 0.5f, alpha_color(TH->border, 90));
	fill_round(sw - (float)S(8), y, sw - (float)S(8) + bar, y + thumb_h,
		bar * 0.5f, alpha_color(TH->muted, 150));
}

static void draw_sidebar_pages(UiRect content, int pad, float sw)
{
	ImGuiIO *io = igGetIO_Nil();
	float cell_x0 = content.x0 + (float)pad;
	float cell_x1 = sw - (float)pad - (float)S(5);
	float cell_w = cell_x1 - cell_x0;
	float gap = (float)S(8);
	float item_h = (float)S(186);
	float max_h = (float)S(152);
	float total;
	float view_h = content.y1 - content.y0;
	int count = A->doc.page_count;
	int first;
	int last;
	int rendered = 0;
	int i;

	total = (float)S(12) * 2.0f + (float)count * item_h +
		(float)(count > 1 ? count - 1 : 0) * gap;
	A->sb_scroll_max = total > view_h ? (int)ceilf(total - view_h) : 0;
	if (A->sb_follow && count > 0) {
		float target = (float)S(12) + (float)A->doc.page * (item_h + gap) +
			item_h * 0.5f - view_h * 0.5f;
		if (target < 0.0f)
			target = 0.0f;
		if (target > (float)A->sb_scroll_max)
			target = (float)A->sb_scroll_max;
		A->sb_scroll = (int)lroundf(target);
		A->sb_follow = 0;
	}
	if (A->sb_scroll < 0)
		A->sb_scroll = 0;
	if (A->sb_scroll > A->sb_scroll_max)
		A->sb_scroll = A->sb_scroll_max;

	if (!g_modal && !g_settings_open &&
		igIsMouseHoveringRect(v2(content.x0, content.y0),
			v2(content.x1, content.y1), 1) && io->MouseWheel != 0.0f) {
		A->sb_scroll -= (int)lroundf(io->MouseWheel * (float)S(110));
		if (A->sb_scroll < 0)
			A->sb_scroll = 0;
		if (A->sb_scroll > A->sb_scroll_max)
			A->sb_scroll = A->sb_scroll_max;
		A->sb_follow = 0;
	}

	first = (int)floorf(((float)A->sb_scroll - (float)S(12)) / (item_h + gap));
	if (first < 0)
		first = 0;
	last = (int)floorf(((float)A->sb_scroll + view_h - (float)S(12)) /
		(item_h + gap));
	if (last > count - 1)
		last = count - 1;

	ImDrawList_PushClipRect(DL, v2(content.x0, content.y0),
		v2(content.x1, content.y1), true);
	for (i = first; i <= last && i < count; i++) {
		float cell_y = content.y0 + (float)S(12) +
			(float)i * (item_h + gap) - (float)A->sb_scroll;
		int current = i == A->doc.page;
		int hovered = 0;
		int clicked = 0;
		int slot;
		char id[40];
		char label[40];

		if (!g_settings_open) {
			float vy0 = cell_y > content.y0 ? cell_y : content.y0;
			float vy1 = cell_y + item_h < content.y1 ? cell_y + item_h : content.y1;
			if (vy1 - vy0 > 2.0f) {
				snprintf(id, sizeof(id), "##sb_page%d", i);
				igSetCursorScreenPos(v2(cell_x0, vy0));
				igInvisibleButton(id, v2(cell_w, vy1 - vy0),
					ImGuiButtonFlags_MouseButtonLeft);
				hovered = igIsItemHovered(ImGuiHoveredFlags_None) ? 1 : 0;
				clicked = igIsItemClicked(ImGuiMouseButton_Left) ? 1 : 0;
				if (hovered && igIsItemHovered(ImGuiHoveredFlags_DelayNormal))
					tooltip_show("Ir para esta página");
			}
		}
		if (current)
			fill_round(cell_x0, cell_y, cell_x1, cell_y + item_h,
				(float)S(8), TH->accent_soft);
		else if (hovered)
			fill_round(cell_x0, cell_y, cell_x1, cell_y + item_h,
				(float)S(8), TH->hover);

		slot = sidebar_thumb_slot(A, i);
		if (slot < 0 && !rendered) {
			sidebar_thumb_render(A, i, first, last);
			slot = sidebar_thumb_slot(A, i);
			rendered = 1;
		}
		if (slot >= 0) {
			ThumbEntry *entry = &A->thumbs[slot];
			float box_y = cell_y + (float)S(7);
			A->thumbs[slot].used = ++A->thumb_clock;
			if (entry->w > 0 && entry->h > 0) {
				float box_w = cell_w - (float)S(18);
				float sc = box_w / (float)entry->w;
				float ih;
				float iw;
				float ix;
				float iy;
				ImTextureRef_c ref;
				if (max_h / (float)entry->h < sc)
					sc = max_h / (float)entry->h;
				iw = (float)entry->w * sc;
				ih = (float)entry->h * sc;
				ix = cell_x0 + (cell_w - iw) * 0.5f;
				iy = box_y + (max_h - ih) * 0.5f;
				fill_rect(ix, iy, ix + iw, iy + ih, IM_COL32(255, 255, 255, 255));
				ref._TexData = NULL;
				ref._TexID = (ImTextureID)gfx_thumb_texture_id(slot);
				ImDrawList_AddImage(DL, ref, v2(ix, iy), v2(ix + iw, iy + ih),
					v2(0.0f, 0.0f), v2(1.0f, 1.0f), IM_COL32(255, 255, 255, 255));
				stroke_round(ix + 0.5f, iy + 0.5f, ix + iw - 0.5f, iy + ih - 0.5f,
					0.0f, current ? 1.5f : 1.0f,
					current ? TH->accent : TH->border_soft);
			} else {
				fill_round(cell_x0 + (float)S(9), box_y, cell_x1 - (float)S(9),
					box_y + max_h, (float)S(6), TH->field);
				draw_text_center((ImFont *)A->font_ui, T,
					cell_x0 + cell_w * 0.5f, box_y + max_h * 0.5f,
					alpha_color(TH->muted, 160), "\xE2\x80\xA6");
			}
		}
		snprintf(label, sizeof(label), "PÁGINA %d", i + 1);
		draw_text_center((ImFont *)A->font_small, T,
			cell_x0 + cell_w * 0.5f, cell_y + item_h - (float)S(14),
			current ? TH->accent : TH->muted, label);
		if (clicked)
			app_goto_page(A, i);
	}
	ImDrawList_PopClipRect(DL);

	sidebar_scroll_indicator(content, sw);
}

static void draw_sidebar_outline(UiRect content, int pad, float sw)
{
	ImGuiIO *io = igGetIO_Nil();
	float row_h = (float)S(32);
	float x0 = content.x0 + (float)pad;
	float x1 = sw - (float)pad - (float)S(5);
	float text_right = x1 - (float)S(30);
	float view_h = content.y1 - content.y0;
	float number_need = measure((ImFont *)A->font_ui, T, "0000").x +
		(float)S(12);
	int count;
	int first;
	int last;
	int i;
	float total;

	if (number_need > (float)S(30))
		text_right = x1 - number_need;
	if (!A->outline_loaded) {
		A->outline_count = doc_load_outline(&A->doc, A->outline, MAX_OUTLINE_ITEMS);
		A->outline_loaded = 1;
	}
	count = A->outline_count;
	total = (float)S(12) * 2.0f + (float)count * row_h;
	A->sb_scroll_max = total > view_h ? (int)ceilf(total - view_h) : 0;
	if (A->sb_follow && count > 0) {
		int target_index = -1;
		float target;
		for (i = 0; i < count; i++) {
			if (A->outline[i].page == A->doc.page) {
				target_index = i;
				break;
			}
		}
		if (target_index >= 0) {
			target = (float)S(12) + (float)target_index * row_h +
				row_h * 0.5f - view_h * 0.5f;
			if (target < 0.0f)
				target = 0.0f;
			if (target > (float)A->sb_scroll_max)
				target = (float)A->sb_scroll_max;
			A->sb_scroll = (int)lroundf(target);
		}
		A->sb_follow = 0;
	}
	if (A->sb_scroll < 0)
		A->sb_scroll = 0;
	if (A->sb_scroll > A->sb_scroll_max)
		A->sb_scroll = A->sb_scroll_max;

	if (!g_modal && !g_settings_open &&
		igIsMouseHoveringRect(v2(content.x0, content.y0),
			v2(content.x1, content.y1), 1) && io->MouseWheel != 0.0f) {
		A->sb_scroll -= (int)lroundf(io->MouseWheel * (float)S(110));
		if (A->sb_scroll < 0)
			A->sb_scroll = 0;
		if (A->sb_scroll > A->sb_scroll_max)
			A->sb_scroll = A->sb_scroll_max;
		A->sb_follow = 0;
	}

	if (count == 0) {
		draw_text_center((ImFont *)A->font_ui, T,
			(content.x0 + content.x1) * 0.5f, content.y0 + view_h * 0.5f - (float)S(14),
			TH->muted, "Sem sumário neste PDF.");
		draw_text_center((ImFont *)A->font_small, T,
			(content.x0 + content.x1) * 0.5f, content.y0 + view_h * 0.5f + (float)S(16),
			alpha_color(TH->muted, 170), "Use as miniaturas ao lado.");
		return;
	}

	first = (int)floorf(((float)A->sb_scroll - (float)S(12)) / row_h);
	if (first < 0)
		first = 0;
	last = (int)floorf(((float)A->sb_scroll + view_h - (float)S(12)) / row_h);
	if (last > count - 1)
		last = count - 1;

	ImDrawList_PushClipRect(DL, v2(content.x0, content.y0),
		v2(content.x1, content.y1), true);
	for (i = first; i <= last && i < count; i++) {
		DocOutlineItem *item = &A->outline[i];
		float row_y = content.y0 + (float)S(12) + (float)i * row_h -
			(float)A->sb_scroll;
		float indent = (float)item->depth * (float)S(13);
		int current = item->page >= 0 && item->page == A->doc.page;
		int hovered = 0;
		int clicked = 0;
		char id[32];
		char number[24];

		if (indent > (x1 - x0) * 0.4f)
			indent = (x1 - x0) * 0.4f;
		if (!g_settings_open && item->page >= 0) {
			float vy0 = row_y > content.y0 ? row_y : content.y0;
			float vy1 = row_y + row_h < content.y1 ? row_y + row_h : content.y1;
			if (vy1 - vy0 > 2.0f) {
				snprintf(id, sizeof(id), "##sb_out%d", i);
				igSetCursorScreenPos(v2(x0, vy0));
				igInvisibleButton(id, v2(x1 - x0, vy1 - vy0),
					ImGuiButtonFlags_MouseButtonLeft);
				hovered = igIsItemHovered(ImGuiHoveredFlags_None) ? 1 : 0;
				clicked = igIsItemClicked(ImGuiMouseButton_Left) ? 1 : 0;
			}
		}
		if (current)
			fill_round(x0, row_y + 1.0f, x1, row_y + row_h - 1.0f,
				(float)S(6), TH->accent_soft);
		else if (hovered)
			fill_round(x0, row_y + 1.0f, x1, row_y + row_h - 1.0f,
				(float)S(6), TH->hover);
		if (current)
			fill_rect(x0 + 0.5f, row_y + (float)S(7), x0 + 2.0f,
				row_y + row_h - (float)S(7), TH->accent);
		draw_text_ellipsis((ImFont *)A->font_ui, T,
			x0 + (float)S(8) + indent, row_y + row_h * 0.5f,
			text_right - (x0 + (float)S(8) + indent),
			current ? TH->accent : (item->page >= 0 ? TH->ink : TH->muted),
			u8w(item->title));
		if (item->page >= 0) {
			snprintf(number, sizeof(number), "%d", item->page + 1);
			draw_text_right((ImFont *)A->font_small, T, x1 - (float)S(8),
				row_y + row_h * 0.5f, alpha_color(TH->muted, 190), number);
		}
		if (clicked)
			app_goto_page(A, item->page);
	}
	ImDrawList_PopClipRect(DL);

	sidebar_scroll_indicator(content, sw);
}

static int sidebar_tab_button(const char *id, const char *label, float x, float y,
	float w, float h, int active)
{
	int clicked;
	int hovered;

	igSetCursorScreenPos(v2(x, y));
	clicked = igInvisibleButton(id, v2(w, h), ImGuiButtonFlags_MouseButtonLeft) ? 1 : 0;
	hovered = igIsItemHovered(ImGuiHoveredFlags_None) ? 1 : 0;
	if (active)
		fill_round(x, y, x + w, y + h, (float)S(7), TH->accent_soft);
	else if (hovered)
		fill_round(x, y, x + w, y + h, (float)S(7), TH->hover);
	draw_text_center((ImFont *)A->font_ui, T, x + w * 0.5f,
		y + h * 0.5f, active ? TH->accent : (hovered ? TH->ink : TH->muted), label);
	return clicked;
}

static void draw_sidebar(float sw, float top, float bottom)
{
	int pad = S(12);
	float header_h = (float)S(48);
	float tab_h = (float)S(30);
	float tab_y = top + (header_h - tab_h) * 0.5f;
	float close_sz = (float)S(24);
	float close_x = sw - (float)S(10) - close_sz;
	float tabs_w = close_x - (float)S(8) - (float)S(10);
	float tab_w = (tabs_w - (float)S(6)) * 0.5f;
	UiRect content;

	fill_rect(0.0f, top, sw, bottom, TH->chrome);
	fill_rect(sw - 1.0f, top, sw, bottom, TH->border);
	fill_rect(0.0f, top + header_h - 1.0f, sw, top + header_h, TH->border);

	if (sidebar_tab_button("##sbtab_pages", "PÁGINAS", (float)S(10), tab_y,
		tab_w, tab_h, A->sidebar_tab == 0) && A->sidebar_tab != 0) {
		A->sidebar_tab = 0;
		A->sb_scroll = 0;
		A->sb_follow = 1;
	}
	if (sidebar_tab_button("##sbtab_outline", "SUMÁRIO",
		(float)S(10) + tab_w + (float)S(6), tab_y, tab_w, tab_h,
		A->sidebar_tab == 1) && A->sidebar_tab != 1) {
		A->sidebar_tab = 1;
		A->sb_scroll = 0;
		A->sb_follow = 1;
	}
	if (tool_button("##sb_close", G.close_small, close_x,
		top + (header_h - close_sz) * 0.5f, close_sz, T * 0.55f,
		"Fechar painel (Ctrl+B)", 1, 0, 0))
		app_toggle_sidebar(A);

	content = rectf(0.0f, top + header_h, sw, bottom);
	if (A->sidebar_tab == 0)
		draw_sidebar_pages(content, pad, sw);
	else
		draw_sidebar_outline(content, pad, sw);
}

static void draw_scrollbars(UiRect view)
{
	ImGuiIO *io = igGetIO_Nil();
	float bar = (float)S(7);
	float edge = (float)S(4);

	if (A->scroll_max > 0) {
		float track_x = view.x1 - bar - edge;
		float track_y = view.y0 + (float)S(5);
		float track_h = (view.y1 - view.y0) - (float)S(10) -
			(A->scroll_max_x > 0 ? bar + edge : 0.0f);
		float thumb_h = track_h * (view.y1 - view.y0) /
			((float)A->doc.page_h + (float)S(VIEW_MARGIN) * 2.0f);
		float thumb_y;
		int hovered;
		if (thumb_h < (float)S(30))
			thumb_h = (float)S(30);
		if (thumb_h > track_h)
			thumb_h = track_h;
		thumb_y = track_y + (track_h - thumb_h) *
			((float)A->scroll_y / (float)A->scroll_max);
		igSetCursorScreenPos(v2(track_x, track_y));
		igInvisibleButton("##vscroll", v2(bar, track_h), ImGuiButtonFlags_MouseButtonLeft);
		hovered = (igIsItemHovered(ImGuiHoveredFlags_None) || igIsItemActive()) ? 1 : 0;
		if (igIsItemActive()) {
			float rel = (io->MousePos.y - track_y - thumb_h * 0.5f) /
				(track_h - thumb_h);
			if (rel < 0.0f)
				rel = 0.0f;
			if (rel > 1.0f)
				rel = 1.0f;
			A->scroll_y = (int)lroundf(rel * (float)A->scroll_max);
		}
		fill_round(track_x, track_y, track_x + bar, track_y + track_h, bar * 0.5f,
			alpha_color(TH->border, hovered ? 150 : 90));
		fill_round(track_x, thumb_y, track_x + bar, thumb_y + thumb_h, bar * 0.5f,
			alpha_color(TH->muted, hovered ? 210 : 150));
		if (hovered)
			igSetMouseCursor(ImGuiMouseCursor_ResizeNS);
	}
	if (A->scroll_max_x > 0) {
		float track_x = view.x0 + (float)S(5);
		float track_y = view.y1 - bar - edge;
		float track_w = (view.x1 - view.x0) - (float)S(10) -
			(A->scroll_max > 0 ? bar + edge : 0.0f);
		float thumb_w = track_w * (view.x1 - view.x0) /
			((float)A->doc.page_w + (float)S(VIEW_MARGIN) * 2.0f);
		float thumb_x;
		int hovered;
		if (thumb_w < (float)S(30))
			thumb_w = (float)S(30);
		if (thumb_w > track_w)
			thumb_w = track_w;
		thumb_x = track_x + (track_w - thumb_w) *
			((float)A->scroll_x / (float)A->scroll_max_x);
		igSetCursorScreenPos(v2(track_x, track_y));
		igInvisibleButton("##hscroll", v2(track_w, bar), ImGuiButtonFlags_MouseButtonLeft);
		hovered = (igIsItemHovered(ImGuiHoveredFlags_None) || igIsItemActive()) ? 1 : 0;
		if (igIsItemActive()) {
			float rel = (io->MousePos.x - track_x - thumb_w * 0.5f) /
				(track_w - thumb_w);
			if (rel < 0.0f)
				rel = 0.0f;
			if (rel > 1.0f)
				rel = 1.0f;
			A->scroll_x = (int)lroundf(rel * (float)A->scroll_max_x);
		}
		fill_round(track_x, track_y, track_x + track_w, track_y + bar, bar * 0.5f,
			alpha_color(TH->border, hovered ? 150 : 90));
		fill_round(thumb_x, track_y, thumb_x + thumb_w, track_y + bar, bar * 0.5f,
			alpha_color(TH->muted, hovered ? 210 : 150));
		if (hovered)
			igSetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}
}

static void draw_canvas(UiRect view)
{
	ImGuiIO *io = igGetIO_Nil();
	Doc *doc = &A->doc;
	int sb_area = S(14);
	int vscroll = A->scroll_max > 0;
	int hscroll = A->scroll_max_x > 0;
	UiRect page_area = view;
	ImVec2_c mouse = io->MousePos;
	int over_scroll = 0;
	int page_hover;

	fill_rect(view.x0, view.y0, view.x1, view.y1, TH->canvas);
	if (!doc->doc) {
		draw_empty_state();
		return;
	}

	update_page_rect(view);
	page_area = view;
	if (vscroll)
		page_area.x1 -= (float)sb_area;
	if (hscroll)
		page_area.y1 -= (float)sb_area;

	if (!g_modal && !g_settings_open &&
		igIsMouseHoveringRect(v2(page_area.x0, page_area.y0),
		v2(page_area.x1, page_area.y1), 1)) {
		if (io->KeyCtrl && io->MouseWheel != 0.0f) {
			float factor = io->MouseWheel > 0.0f ? 1.15f : 0.87f;
			float target = doc->zoom * factor;
			if (target < MIN_ZOOM)
				target = MIN_ZOOM;
			if (target > MAX_ZOOM)
				target = MAX_ZOOM;
			app_set_zoom(A, target);
		} else if (io->MouseWheel != 0.0f) {
			if (io->KeyShift)
				A->scroll_x -= (int)lroundf(io->MouseWheel * (float)S(84));
			else
				A->scroll_y -= (int)lroundf(io->MouseWheel * (float)S(84));
		}
	}

	{
		if (vscroll && mouse.x > page_area.x1 && mouse.y > page_area.y0 &&
			mouse.y < page_area.y1)
			over_scroll = 1;
		if (hscroll && mouse.y > page_area.y1 && mouse.x > page_area.x0 &&
			mouse.x < page_area.x1)
			over_scroll = 1;
	}

	if (!g_modal && !g_settings_open) {
		page_hover = igIsMouseHoveringRect(v2((float)A->rc_page.left, (float)A->rc_page.top),
			v2((float)A->rc_page.right, (float)A->rc_page.bottom), 1) && !over_scroll;
		if (page_hover && doc->has_text)
			igSetMouseCursor(ImGuiMouseCursor_TextInput);
		if (page_hover && io->MouseClicked[0]) {
			if (!doc->has_text) {
				if (app_status_seconds(A) <= 0)
					app_set_status(A, 1, L"Esta página é uma imagem. Use o OCR para obter o texto.");
			} else if (io->MouseDoubleClicked[0]) {
				fz_point pa, pb;
				fz_quad quad;
				fz_point point = screen_to_page(mouse);
				if (doc_word_at(doc, point, &pa, &pb, &quad)) {
					A->sel_a = pa;
					A->sel_b = pb;
					A->sel_quads[0] = quad;
					A->sel_count = 1;
					A->has_selection = 1;
					A->selecting = 0;
				}
			} else {
				fz_point point = screen_to_page(mouse);
				A->sel_a = point;
				A->sel_b = point;
				A->sel_count = 0;
				A->has_selection = 0;
				A->selecting = 1;
			}
		}
		if (A->selecting && io->MouseDown[0]) {
			fz_point point = screen_to_page(mouse);
			if (point.x != A->sel_b.x || point.y != A->sel_b.y) {
				A->sel_b = point;
				A->sel_count = doc_selection_quads(doc, A->sel_a, A->sel_b,
					A->sel_quads, MAX_SEL_QUADS);
				A->has_selection = A->sel_count > 0;
			}
		}
		if (A->selecting && !io->MouseDown[0]) {
			A->selecting = 0;
			A->has_selection = A->sel_count > 0;
		}
	}

	if (A->rc_page.right > A->rc_page.left && A->rc_page.bottom > A->rc_page.top) {
		ImDrawList_PushClipRect(DL, v2(view.x0, view.y0), v2(view.x1, view.y1), true);
		{
			int i;
			for (i = S(8); i >= 1; i--) {
				int alpha = 8 + (S(8) - i) * 2;
				if (alpha > 24)
					alpha = 24;
				fill_round((float)A->rc_page.left - (float)i + (float)S(2),
					(float)A->rc_page.top - (float)i + (float)S(4),
					(float)A->rc_page.right + (float)i + (float)S(2),
					(float)A->rc_page.bottom + (float)i + (float)S(4),
					3.0f, alpha_color(TH->shadow, alpha));
			}
		}
		fill_rect((float)A->rc_page.left, (float)A->rc_page.top, (float)A->rc_page.right,
			(float)A->rc_page.bottom, IM_COL32(255, 255, 255, 255));
		if (A->tex_id && A->tex_page == doc->page) {
			ImTextureRef_c ref;
			ref._TexData = NULL;
			ref._TexID = (ImTextureID)A->tex_id;
			ImDrawList_AddImage(DL, ref,
				v2((float)A->rc_page.left, (float)A->rc_page.top),
				v2((float)A->rc_page.right, (float)A->rc_page.bottom),
				v2(0.0f, 0.0f), v2(1.0f, 1.0f), IM_COL32(255, 255, 255, 255));
		}
		ImDrawList_PushClipRect(DL, v2((float)A->rc_page.left, (float)A->rc_page.top),
			v2((float)A->rc_page.right, (float)A->rc_page.bottom), true);
		{
			int i;
			for (i = 0; i < A->mark_count; i++) {
				Mark *mark = &A->marks[i];
				int j;
				if (mark->page != doc->page)
					continue;
				for (j = 0; j < mark->quad_count; j++)
					add_quad(mark->quads[j], IM_COL32(0xF2, 0xC5, 0x4C, 105));
			}
			for (i = 0; i < A->hit_count; i++) {
				if (A->hits[i].page != doc->page)
					continue;
				if (i == A->hit_index) {
					add_quad(A->hits[i].quad, alpha_color(TH->search, 125));
					{
						ImVec2_c p0 = page_to_screen(A->hits[i].quad.ul);
						ImVec2_c p1 = page_to_screen(A->hits[i].quad.ur);
						ImVec2_c p2 = page_to_screen(A->hits[i].quad.lr);
						ImVec2_c p3 = page_to_screen(A->hits[i].quad.ll);
						ImDrawList_AddQuad(DL, p0, p1, p2, p3, TH->accent,
							(float)S(2) * 0.5f + 1.0f, 0);
					}
				} else {
					add_quad(A->hits[i].quad, alpha_color(TH->search, 70));
				}
			}
			for (i = 0; i < A->sel_count; i++)
				add_quad(A->sel_quads[i], alpha_color(TH->selection, 72));
		}
		ImDrawList_PopClipRect(DL);
		stroke_round((float)A->rc_page.left + 0.5f, (float)A->rc_page.top + 0.5f,
			(float)A->rc_page.right - 0.5f, (float)A->rc_page.bottom - 0.5f,
			0.0f, 1.0f, TH->border_soft);
		ImDrawList_PopClipRect(DL);

		if (!A->tex_id || A->tex_page != doc->page) {
			float cx = ((float)A->rc_page.left + (float)A->rc_page.right) * 0.5f;
			float cy = ((float)A->rc_page.top + (float)A->rc_page.bottom) * 0.5f;
			draw_text_center((ImFont *)A->font_ui, T, cx, cy, TH->muted,
				doc->error[0] ? u8w(doc->error) : "Renderizando\xE2\x80\xA6");
		}
	} else {
		draw_text_center((ImFont *)A->font_ui, T,
			(view.x0 + view.x1) * 0.5f, (view.y0 + view.y1) * 0.5f, TH->muted,
			doc->error[0] ? u8w(doc->error) : "Renderizando\xE2\x80\xA6");
	}

	draw_scrollbars(view);
}

static void draw_statusbar(float width, float top, int status_h)
{
	const char *left;
	char right[512];
	char extra[64];
	wchar_t right_w[512];
	ImU32 left_color = TH->muted;
	float cy = top + (float)status_h * 0.5f;

	fill_rect(0, top, width, top + (float)status_h, TH->chrome);
	fill_rect(0, top, width, top + 1.0f, TH->border);

	if (A->status[0] && app_status_seconds(A) > 0) {
		left = u8w(A->status);
		left_color = A->status_accent ? TH->error : TH->accent;
	} else if (A->doc.doc) {
		left = A->doc.has_text ? "Texto selecionável disponível" :
			"Sem camada de texto · use o OCR";
	} else {
		left = "Pronto";
	}
	draw_text_ellipsis((ImFont *)A->font_small, T, (float)S(16), cy,
		width * 0.55f, left_color, left);

	if (!A->doc.doc)
		return;
	snprintf(right, sizeof(right), "Página %d de %d  ·  %d%%", A->doc.page + 1,
		A->doc.page_count, (int)lroundf(A->doc.zoom * 100.0f));
	if (A->doc.encrypted)
		strncat(right, "  ·  protegido", sizeof(right) - strlen(right) - 1);
	if (!A->doc.copy_allowed)
		strncat(right, "  ·  cópia restrita", sizeof(right) - strlen(right) - 1);
	if (A->mark_count > 0) {
		snprintf(extra, sizeof(extra),
			A->mark_count == 1 ? "  ·  %d marcação" : "  ·  %d marcações",
			A->mark_count);
		strncat(right, extra, sizeof(right) - strlen(right) - 1);
	}
	if (A->tab_count > 1) {
		snprintf(extra, sizeof(extra), "  ·  aba %d/%d", A->active_tab + 1,
			A->tab_count);
		strncat(right, extra, sizeof(right) - strlen(right) - 1);
	}
	MultiByteToWideChar(CP_UTF8, 0, right, -1, right_w, 512);
	right_w[511] = 0;
	draw_text_right((ImFont *)A->font_small, T, width - (float)S(16),
		cy, TH->muted, u8w(right_w));
}

static void draw_settings_popover(float width, float top, float bottom)
{
	ImGuiIO *io = igGetIO_Nil();
	float pad = (float)S(16);
	float w = (float)S(276);
	float x1 = g_settings_btn.x0 + (float)S(36) + (float)S(8);
	float x0;
	float y0;
	float h;
	float x;
	float cw;
	float y;
	float btn_h = T + (float)S(10);
	float row_h = T + (float)S(16);
	float close_sz = T + (float)S(4);
	const char *hint = "Ctrl+B abre o painel de páginas";
	const char *labels[2] = { "EXPORTAR PÁGINA (.TXT)",
		"EXPORTAR TEXTO DO OCR (.TXT)" };
	int rows = (A->doc.doc ? 1 : 0) + (A->ocr_text ? 1 : 0);
	int i;
	UiRect box;

	if (!g_settings_open)
		return;
	if (btn_h < (float)S(34))
		btn_h = (float)S(34);
	if (row_h < (float)S(40))
		row_h = (float)S(40);
	if (close_sz < (float)S(22))
		close_sz = (float)S(22);
	{
		float need = measure((ImFont *)A->font_ui, T, labels[1]).x + (float)S(44);
		float hint_need = measure((ImFont *)A->font_ui, T, hint).x + (float)S(32);
		if (hint_need > need)
			need = hint_need;
		if (need > w)
			w = need;
	}
	if (x1 > width - (float)S(12))
		x1 = width - (float)S(12);
	x0 = x1 - w;
	if (x0 < (float)S(12)) {
		x0 = (float)S(12);
		x1 = x0 + w;
	}
	if (A->doc.doc && A->sidebar_open &&
		x0 < (float)S(232) + (float)S(8)) {
		x0 = (float)S(232) + (float)S(8);
		x1 = x0 + w;
		if (x1 > width - (float)S(12)) {
			x1 = width - (float)S(12);
			x0 = x1 - w;
		}
	}
	h = (float)S(14) + (T + (float)S(10)) + (float)S(8) + 1.0f +
		(float)S(10) + (T + (float)S(6)) + btn_h + (float)S(10) + 1.0f +
		(float)S(10) + (float)rows * row_h + (T + (float)S(14)) + (float)S(6);
	y0 = top;
	if (y0 + h > bottom - (float)S(6))
		y0 = bottom - (float)S(6) - h;
	if (y0 < (float)S(6))
		y0 = (float)S(6);
	box = rectf(x0, y0, x1, y0 + h);

	fill_round(x0 + (float)S(3), y0 + (float)S(5), x1 + (float)S(3),
		y0 + h + (float)S(5), (float)S(12), alpha_color(TH->shadow, 70));
	fill_round(x0, y0, x1, y0 + h, (float)S(12), TH->surface);
	stroke_round(x0 + 0.5f, y0 + 0.5f, x1 - 0.5f, y0 + h - 0.5f,
		(float)S(12), 1.0f, TH->border);

	x = x0 + pad;
	cw = w - pad * 2.0f;
	y = y0 + (float)S(14);
	draw_text((ImFont *)A->font_bold, T, x, y, TH->ink, "AJUSTES");
	if (tool_button("##set_close", G.close_small, x1 - pad - close_sz,
		y0 + (float)S(12), close_sz, T * 0.55f, "Fechar ajustes", 1, 0, 0))
		g_settings_open = 0;

	y += T + (float)S(10);
	fill_rect(x, y, x + cw, y + 1.0f, TH->border_soft);
	y += (float)S(8) + 1.0f + (float)S(10);
	draw_text((ImFont *)A->font_small, T, x, y, TH->muted,
		"ESCALA DA INTERFACE");
	y += T + (float)S(6);
	{
		float gap = (float)S(5);
		float bw = (cw - gap * 2.0f) / 3.0f;
		static const int pcts[3] = { 100, 110, 125 };
		for (i = 0; i < 3; i++) {
			char id[32];
			char label[16];
			snprintf(id, sizeof(id), "##set_scale%d", pcts[i]);
			snprintf(label, sizeof(label), "%d%%", pcts[i]);
			if (text_button(id, label, x + (float)i * (bw + gap), y, bw,
				btn_h, A->ui_scale_pct == pcts[i]))
				app_set_ui_scale(A, pcts[i]);
		}
	}
	y += btn_h + (float)S(10);
	fill_rect(x, y, x + cw, y + 1.0f, TH->border_soft);
	y += 1.0f + (float)S(10);
	if (A->doc.doc) {
		if (text_button("##set_export_page", labels[0], x, y,
			cw, btn_h, 0)) {
			g_settings_open = 0;
			app_export_page_text(A);
		}
		y += row_h;
	}
	if (A->ocr_text) {
		if (text_button("##set_export_ocr", labels[1], x, y,
			cw, btn_h, 0)) {
			g_settings_open = 0;
			app_export_ocr_text(A);
		}
		y += row_h;
	}
	draw_text((ImFont *)A->font_small, T, x,
		y0 + h - (float)S(6) - T, alpha_color(TH->muted, 190), hint);

	if (g_settings_age > 0) {
		g_settings_age--;
	} else if (igIsMouseClicked_Bool(ImGuiMouseButton_Left, false) &&
		!in_rect(box, io->MousePos) && !in_rect(g_settings_btn, io->MousePos)) {
		g_settings_open = 0;
	}
}

static void draw_password_modal(void)
{
	ImGuiIO *io = igGetIO_Nil();
	float width = (float)S(430);
	float height = (float)S(258);
	float cx = io->DisplaySize.x * 0.5f;
	float cy = io->DisplaySize.y * 0.5f;
	float pad = (float)S(26);
	float rounding = (float)S(13);
	ImDrawList *saved = DL;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;

	igSetNextWindowPos(v2(cx, cy), ImGuiCond_Always, v2(0.5f, 0.5f));
	igSetNextWindowSize(v2(width, height), ImGuiCond_Always);
	if (!g_password_was_open)
		igSetNextWindowFocus();
	if (!igBegin("##password_modal", NULL, flags)) {
		igEnd();
		return;
	}
	DL = igGetWindowDrawList();
	{
		ImVec2_c pos = igGetWindowPos();
		float x = pos.x + pad;
		float y = pos.y + pad;
		int enter;

		fill_round(pos.x, pos.y, pos.x + width, pos.y + height, rounding, TH->surface);
		stroke_round(pos.x + 0.5f, pos.y + 0.5f, pos.x + width - 0.5f,
			pos.y + height - 0.5f, rounding, 1.0f, TH->border);
		draw_mark(x, y, (float)S(30));
		draw_text((ImFont *)A->font_bold, T, x + (float)S(42),
			y - 1.0f * U, TH->ink, "PDF PROTEGIDO");
		draw_text((ImFont *)A->font_ui, T, x + (float)S(42),
			y + T + (float)S(6), TH->muted, "Este PDF exige uma senha de abertura.");

		y += T * 2.0f + (float)S(22);
		draw_text((ImFont *)A->font_bold, T, x, y, TH->muted, "SENHA");
		y += T + (float)S(4);
		igSetCursorScreenPos(v2(x, y));
		igSetNextItemWidth(width - pad * 2.0f);
		igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, v2((float)S(9), (float)S(8)));
		if (A->password_focus) {
			igSetKeyboardFocusHere(0);
			A->password_focus = 0;
		}
		enter = igInputText("##password", A->password, sizeof(A->password),
			ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue,
			NULL, NULL) ? 1 : 0;
		igPopStyleVar(1);
		if (enter)
			app_submit_password(A);

		y += T + (float)S(24);
		if (A->password_error[0])
			draw_text_ellipsis((ImFont *)A->font_small, T, x, y,
				width - pad * 2.0f, TH->error, u8w(A->password_error));

		{
			float btn_h = T + (float)S(12);
			float open_w = button_width("ABRIR", 104);
			float cancel_w = button_width("CANCELAR", 104);
			float by;
			float bx;
			if (btn_h < (float)S(36))
				btn_h = (float)S(36);
			by = pos.y + height - pad - btn_h;
			bx = pos.x + width - pad - open_w;
			if (text_button("##pwd_open", "ABRIR", bx, by, open_w,
				btn_h, 1))
				app_submit_password(A);
			if (text_button("##pwd_cancel", "CANCELAR", bx - (float)S(10) - cancel_w,
				by, cancel_w, btn_h, 0))
				app_cancel_password(A);
		}
	}
	igEnd();
	DL = saved;
	g_password_was_open = 1;
}

static void close_ocr_modal(void)
{
	A->ocr_popup = 0;
	A->ocr_popup_shown = 0;
	free(A->ocr_utf8);
	A->ocr_utf8 = NULL;
	g_ocr_seen = NULL;
}

static void draw_ocr_modal(void)
{
	ImGuiIO *io = igGetIO_Nil();
	float width = (float)S(690);
	float height = (float)S(520);
	float cx = io->DisplaySize.x * 0.5f;
	float cy = io->DisplaySize.y * 0.5f;
	float pad = (float)S(24);
	float rounding = (float)S(13);
	float field_w;
	float text_h;
	float btn_h = T + (float)S(12);
	float header_h = T * 2.0f + (float)S(22);
	ImDrawList *saved = DL;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
	size_t text_len;
	char subtitle[128];

	if (width > io->DisplaySize.x - (float)S(80))
		width = io->DisplaySize.x - (float)S(80);
	if (height > io->DisplaySize.y - (float)S(80))
		height = io->DisplaySize.y - (float)S(80);
	if (width < (float)S(420))
		width = (float)S(420);
	if (height < (float)S(300))
		height = (float)S(300);
	if (!A->ocr_popup_shown) {
		free(A->ocr_utf8);
		A->ocr_utf8 = mu_wide_to_utf8_alloc(A->ocr_text ? A->ocr_text : L"");
		g_ocr_seen = A->ocr_text;
		A->ocr_popup_shown = 1;
	} else if (g_ocr_seen != A->ocr_text) {
		free(A->ocr_utf8);
		A->ocr_utf8 = mu_wide_to_utf8_alloc(A->ocr_text ? A->ocr_text : L"");
		g_ocr_seen = A->ocr_text;
	}
	text_len = A->ocr_utf8 ? strlen(A->ocr_utf8) + 1 : 1;
	if (btn_h < (float)S(36))
		btn_h = (float)S(36);
	field_w = width - pad * 2.0f;
	text_h = height - pad * 2.0f - btn_h - (float)S(12) - header_h;
	if (text_h < (float)S(120))
		text_h = (float)S(120);

	igSetNextWindowPos(v2(cx, cy), ImGuiCond_Always, v2(0.5f, 0.5f));
	igSetNextWindowSize(v2(width, height), ImGuiCond_Always);
	if (!g_ocr_was_open)
		igSetNextWindowFocus();
	if (!igBegin("##ocr_modal", NULL, flags)) {
		igEnd();
		return;
	}
	DL = igGetWindowDrawList();
	{
		ImVec2_c pos = igGetWindowPos();
		float x = pos.x + pad;
		float y = pos.y + pad;

		fill_round(pos.x, pos.y, pos.x + width, pos.y + height, rounding, TH->surface);
		stroke_round(pos.x + 0.5f, pos.y + 0.5f, pos.x + width - 0.5f,
			pos.y + height - 0.5f, rounding, 1.0f, TH->border);
		draw_mark(x, y, (float)S(30));
		draw_text((ImFont *)A->font_bold, T, x + (float)S(42),
			y - 1.0f * U, TH->ink, "TEXTO RECONHECIDO");
		snprintf(subtitle, sizeof(subtitle), "Página %d  ·  OCR local do Windows",
			A->ocr_page + 1);
		draw_text((ImFont *)A->font_ui, T, x + (float)S(42),
			y + T + (float)S(6), TH->muted, subtitle);

		y += header_h;
		igSetCursorScreenPos(v2(x, y));
		igSetNextItemWidth(field_w);
		igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, v2((float)S(10), (float)S(9)));
		igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, (float)S(9));
		igInputTextMultiline("##ocrtext", A->ocr_utf8 ? A->ocr_utf8 : "",
			text_len, v2(field_w, text_h), ImGuiInputTextFlags_ReadOnly,
			NULL, NULL);
		igPopStyleVar(2);
		stroke_round(x + 0.5f, y + 0.5f, x + field_w - 0.5f, y + text_h - 0.5f,
			(float)S(9), 1.0f, TH->border);

		{
			float copy_w = button_width("COPIAR TEXTO", 150);
			float close_w = button_width("FECHAR", 104);
			float export_w = button_width("EXPORTAR .TXT", 124);
			float by = pos.y + height - pad - btn_h;
			float bx = pos.x + width - pad - copy_w;
			if (A->ocr_focus_copied == 0) {
				igSetCursorScreenPos(v2(bx, by));
				igSetKeyboardFocusHere(0);
				A->ocr_focus_copied = 1;
			}
			if (text_button("##ocr_copy", "COPIAR TEXTO", bx, by,
				copy_w, btn_h, 1)) {
				igSetClipboardText(A->ocr_utf8 ? A->ocr_utf8 : "");
				app_set_status(A, 0, L"%s", L"Texto do OCR copiado");
				A->ocr_focus_copied = 2;
			}
			if (text_button("##ocr_export", "EXPORTAR .TXT",
				bx - (float)S(10) - close_w - (float)S(10) - export_w,
				by, export_w, btn_h, 0))
				app_export_ocr_text(A);
			if (text_button("##ocr_close", "FECHAR",
				bx - (float)S(10) - close_w, by, close_w,
				btn_h, 0))
				close_ocr_modal();
		}
	}
	igEnd();
	DL = saved;
	g_ocr_was_open = 1;
}

static void about_section(const char *title)
{
	igDummy(v2(0.0f, (float)S(6)));
	igPushFont((ImFont *)A->font_bold, T);
	igPushStyleColor_U32(ImGuiCol_Text, TH->accent);
	igTextUnformatted(title, NULL);
	igPopStyleColor(1);
	igPopFont();
	igDummy(v2(0.0f, (float)S(2)));
}

static void about_item(const char *text)
{
	igPushFont((ImFont *)A->font_ui, T);
	igPushStyleColor_U32(ImGuiCol_Text, TH->ink);
	igTextWrapped("%s", text);
	igPopStyleColor(1);
	igPopFont();
}

static void draw_about_modal(void)
{
	ImGuiIO *io = igGetIO_Nil();
	float width = (float)S(860);
	float height = (float)S(680);
	float cx = io->DisplaySize.x * 0.5f;
	float cy = io->DisplaySize.y * 0.5f;
	float pad = (float)S(24);
	float rounding = (float)S(13);
	float field_w;
	float x;
	float y;
	float content_y;
	float content_h;
	float footer_by;
	float credit_y;
	float sep_y;
	float link_h = T + (float)S(14);
	float size_h = T + (float)S(12);
	float header_advance = T * 2.0f + (float)S(22);
	static const int size_values[4] = { 15, 17, 19, 21 };
	static const char *size_names[4] = { "PEQUENO", "PADRÃO", "GRANDE", "MAIOR" };
	ImDrawList *saved = DL;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;

	if (width > io->DisplaySize.x - (float)S(60))
		width = io->DisplaySize.x - (float)S(60);
	if (height > io->DisplaySize.y - (float)S(24))
		height = io->DisplaySize.y - (float)S(24);
	if (link_h < (float)S(36))
		link_h = (float)S(36);
	if (size_h < (float)S(32))
		size_h = (float)S(32);
	field_w = width - pad * 2.0f;

	igSetNextWindowPos(v2(cx, cy), ImGuiCond_Always, v2(0.5f, 0.5f));
	igSetNextWindowSize(v2(width, height), ImGuiCond_Always);
	if (!igBegin("##about_modal", NULL, flags)) {
		igEnd();
		return;
	}
	DL = igGetWindowDrawList();
	{
		ImVec2_c pos = igGetWindowPos();
		float col_w;

		x = pos.x + pad;
		y = pos.y + pad;
		fill_round(pos.x, pos.y, pos.x + width, pos.y + height, rounding, TH->surface);
		stroke_round(pos.x + 0.5f, pos.y + 0.5f, pos.x + width - 0.5f,
			pos.y + height - 0.5f, rounding, 1.0f, TH->border);
		draw_mark(x, y, (float)S(38));
		draw_text((ImFont *)A->font_bold, T, x + (float)S(50),
			y + 1.0f * U, TH->ink, "SOBRE O PDF LEVE");
		draw_text_right((ImFont *)A->font_small, T, pos.x + width - pad,
			y + T * 0.5f, TH->muted, "VERSÃO 1.1");
		draw_text((ImFont *)A->font_ui, T, x + (float)S(50),
			y + T + (float)S(8), TH->muted,
			"Todas as funções, atalhos e privacidade do leitor.");

		y += header_advance;
		fill_rect(x, y, pos.x + width - pad, y + 1.0f, TH->border_soft);
		y += (float)S(14);
		{
			float label_w = measure((ImFont *)A->font_small, T,
				"TAMANHO DO TEXTO").x;
			float gap = (float)S(5);
			float widths[4];
			float total = 0.0f;
			float bx;
			float cy = y + size_h * 0.5f;
			int i;
			for (i = 0; i < 4; i++) {
				widths[i] = measure((ImFont *)A->font_ui, T, size_names[i]).x +
					(float)S(20);
				total += widths[i];
			}
			total += gap * 3.0f;
			bx = pos.x + width - pad - total;
			if (bx < x + label_w + (float)S(12))
				bx = x + label_w + (float)S(12);
			draw_text((ImFont *)A->font_small, T, x,
				cy - T * 0.5f, TH->muted, "TAMANHO DO TEXTO");
			for (i = 0; i < 4; i++) {
				char id[32];
				snprintf(id, sizeof(id), "##about_text%d", size_values[i]);
				if (text_button(id, size_names[i], bx, y, widths[i], size_h,
					A->text_size == size_values[i]))
					app_set_text_size(A, size_values[i]);
				bx += widths[i] + gap;
			}
		}
		y += size_h + (float)S(14);
		draw_text((ImFont *)A->font_small, T, x, y, TH->muted, "TODOS OS RECURSOS");
		content_y = y + T + (float)S(8);

		footer_by = pos.y + height - pad - link_h;
		credit_y = footer_by - T - (float)S(10);
		sep_y = credit_y - (float)S(12);
		content_h = sep_y - (float)S(10) - content_y;
		if (content_h < (float)S(80))
			content_h = (float)S(80);

		igSetCursorScreenPos(v2(x, content_y));
		igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, v2((float)S(2), (float)S(2)));
		igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, v2((float)S(6), (float)S(4)));
		igBeginChild_Str("##about_features", v2(field_w, content_h),
			ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
		col_w = (field_w - (float)S(18)) * 0.5f;
		igColumns(2, "##about_cols", false);
		igSetColumnWidth(0, col_w);
		igSetColumnWidth(1, col_w);

		about_section("LEITURA");
		about_item("• PDFs comuns, com senha e com restrição de cópia; a senha só serve para abrir.");
		about_item("• Abas para vários PDFs; abrir outro PDF usa uma nova aba.");
		about_item("• Tela inicial com PDFs recentes e retomada da última página lida.");
		about_item("• Zoom livre, 100%, ajuste à largura ou à página, rolagem contínua e giro de 90°.");
		about_item("• Painel de páginas com miniaturas e sumário, aberto com Ctrl+B.");
		about_item("• Tamanho do texto em quatro opções e escala da interface em 100%, 110% ou 125%.");
		about_item("• Modo claro e escuro, em janela redimensionável.");

		about_section("TEXTO E PESQUISA");
		about_item("• Selecione com o mouse e copie com Ctrl+C ou pelo botão de cópia.");
		about_item("• Selecione a página inteira com Ctrl+A e copie todo o texto.");
		about_item("• Exporte o texto da página ou do OCR para um arquivo .txt.");
		about_item("• Localize com Ctrl+F; veja o contador e pule com F3 ou Shift+F3.");
		about_item("• Marque trechos: o destaque fica salvo fora do PDF (Ctrl+H).");

		about_section("OCR LOCAL");
		about_item("• OCR do Windows, offline, para páginas digitalizadas.");
		about_item("• Texto reconhecido em janela própria, pronto para copiar.");
		about_item("• Cache por página: páginas já reconhecidas não são processadas de novo.");

		igNextColumn();

		about_section("GEMINI (OPCIONAL)");
		about_item("• Resume a página ou a seleção e responde perguntas sobre o documento.");
		about_item("• Chave da API salva criptografada neste computador.");
		about_item("• Envio manual: nada é enviado sem você clicar em Resumir ou Perguntar.");

		about_section("PRIVACIDADE");
		about_item("• Leitura, OCR e marcações acontecem no seu computador.");
		about_item("• Senhas não são armazenadas; servem só para abrir o PDF.");
		about_item("• O Gemini recebe apenas o texto mostrado na janela, quando você envia.");

		about_section("ATALHOS");
		about_item("• Ctrl+O abrir  ·  Ctrl+T nova aba  ·  Ctrl+W fechar");
		about_item("• Ctrl+Tab alternar abas  ·  Ctrl+Shift+D tema");
		about_item("• Ctrl+F localizar  ·  F3 seguinte  ·  Shift+F3 anterior");
		about_item("• Ctrl+A página  ·  Ctrl+C copiar  ·  Ctrl+H marcar");
		about_item("• Ctrl+G Gemini  ·  F1 Sobre  ·  Ctrl+B painel");
		about_item("• Ctrl+0 100%  ·  Ctrl+1 largura  ·  Ctrl+2 página");
		about_item("• Ctrl++ / Ctrl+- zoom  ·  PgUp/PgDn página");
		about_item("• Home / End início e fim do documento");

		igColumns(1, NULL, false);
		igEndChild();
		igPopStyleVar(2);

		fill_rect(x, sep_y, pos.x + width - pad,
			sep_y + 1.0f, TH->border_soft);
		draw_text((ImFont *)A->font_small, T, x, credit_y,
			alpha_color(TH->muted, 200),
			"Desenvolvido por Daniel Ortega Pereira  ·  © 2026");
		{
			float linked_w = button_width("LinkedIn  ·  daniel-op", 190);
			float whats_w = button_width("WhatsApp  ·  +55 (14) 98129-4913", 240);
			float close_w = button_width("FECHAR", 104);
			if (link_button("##about_linkedin", "LinkedIn  ·  daniel-op",
				x, footer_by, linked_w, link_h))
				open_url(APP_LINKEDIN);
			if (link_button("##about_whatsapp", "WhatsApp  ·  +55 (14) 98129-4913",
				x + linked_w + (float)S(8), footer_by, whats_w, link_h))
				open_url(APP_WHATSAPP);
			if (text_button("##about_close", "FECHAR",
				pos.x + width - pad - close_w, footer_by, close_w,
				link_h, 1))
				g_about_open = 0;
		}
	}
	igEnd();
	DL = saved;
}

static void draw_ai_modal(void)
{
	ImGuiIO *io = igGetIO_Nil();
	float width = (float)S(720);
	float height = A->ai_setup ? (float)S(560) : (float)S(620);
	float cx = io->DisplaySize.x * 0.5f;
	float cy = io->DisplaySize.y * 0.5f;
	float pad = (float)S(24);
	float rounding = (float)S(13);
	float field_w;
	float in_h = T + (float)S(16);
	float btn_h = T + (float)S(14);
	float header_advance = T * 2.0f + (float)S(20);
	float x;
	float y;
	float footer_y;
	ImDrawList *saved = DL;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;

	if (width > io->DisplaySize.x - (float)S(80))
		width = io->DisplaySize.x - (float)S(80);
	if (height > io->DisplaySize.y - (float)S(80))
		height = io->DisplaySize.y - (float)S(80);
	if (width < (float)S(440))
		width = (float)S(440);
	if (height < (float)S(340))
		height = (float)S(340);
	if (btn_h < (float)S(36))
		btn_h = (float)S(36);
	if (in_h < (float)S(36))
		in_h = (float)S(36);
	field_w = width - pad * 2.0f;

	igSetNextWindowPos(v2(cx, cy), ImGuiCond_Always, v2(0.5f, 0.5f));
	igSetNextWindowSize(v2(width, height), ImGuiCond_Always);
	if (!g_ai_was_open)
		igSetNextWindowFocus();
	if (!igBegin("##ai_modal", NULL, flags)) {
		igEnd();
		return;
	}
	DL = igGetWindowDrawList();
	{
		ImVec2_c pos = igGetWindowPos();
		char context[96];
		app_ai_context_label(A, context, sizeof(context));
		x = pos.x + pad;
		y = pos.y + pad;
		footer_y = pos.y + height - pad - btn_h;

		fill_round(pos.x, pos.y, pos.x + width, pos.y + height, rounding, TH->surface);
		stroke_round(pos.x + 0.5f, pos.y + 0.5f, pos.x + width - 0.5f,
			pos.y + height - 0.5f, rounding, 1.0f, TH->border);
		draw_ai_mark(x, y, (float)S(30));
		draw_text((ImFont *)A->font_bold, T, x + (float)S(42),
			y - 1.0f * U, TH->ink, "GEMINI");
		draw_text((ImFont *)A->font_ui, T, x + (float)S(42),
			y + T + (float)S(5), TH->muted, A->ai_setup ?
			"Conecte sua chave do Google AI Studio" :
			"Resumo e perguntas sobre este documento");

		if (A->ai_setup) {
			y += header_advance;
			igPushFont((ImFont *)A->font_ui, T);
			igPushStyleColor_U32(ImGuiCol_Text, TH->muted);
			igSetCursorScreenPos(v2(x, y));
			igPushTextWrapPos(pad + field_w);
			igTextUnformatted(
				"A chave fica criptografada neste computador (Windows DPAPI). "
				"Nada é enviado ao Google sem você clicar em Perguntar. "
				"Somente o texto da página ou da seleção escolhida sai deste PC, "
				"e apenas quando você pedir.", NULL);
			igPopTextWrapPos();
			igPopStyleColor(1);
			igPopFont();

			y = igGetCursorScreenPos().y + (float)S(12);
			draw_text((ImFont *)A->font_bold, T, x, y, TH->muted,
				"CHAVE DA API");
			y += T + (float)S(8);
			igSetCursorScreenPos(v2(x, y));
			igSetNextItemWidth(field_w);
			igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, v2((float)S(10), (float)S(9)));
			igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, (float)S(9));
			igPushStyleColor_U32(ImGuiCol_FrameBg, TH->field);
			igPushStyleColor_U32(ImGuiCol_NavCursor, 0);
			in_h = igGetFrameHeight();
			if (A->ai_focus_key) {
				igSetKeyboardFocusHere(0);
				A->ai_focus_key = 0;
			}
			if (igInputTextWithHint("##ai_key", "Cole aqui a chave (começa com AIza…)",
				A->ai_key_input, sizeof(A->ai_key_input),
				ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue,
				NULL, NULL))
				app_ai_save_key(A);
			igPopStyleColor(2);
			igPopStyleVar(2);
			stroke_round(x + 0.5f, y + 0.5f, x + field_w - 0.5f, y + in_h - 0.5f,
				(float)S(9), 1.0f, TH->border);

			if (A->ai_error) {
				igPushFont((ImFont *)A->font_ui, T);
				igPushStyleColor_U32(ImGuiCol_Text, TH->error);
				igSetCursorScreenPos(v2(x, y + in_h + (float)S(10)));
				igPushTextWrapPos(pad + field_w);
				igTextUnformatted(A->ai_error, NULL);
				igPopTextWrapPos();
				igPopStyleColor(1);
				igPopFont();
				y = igGetCursorScreenPos().y + (float)S(10);
			} else {
				y += in_h + (float)S(10);
			}
			draw_text((ImFont *)A->font_small, T, x, y,
				alpha_color(TH->muted, 210),
				"A chave fica só neste computador. Você pode removê-la quando quiser.");
			y += T + (float)S(16);
			draw_text((ImFont *)A->font_bold, T, x, y, TH->muted,
				"COMO FUNCIONA");
			y += T + (float)S(6);
			draw_text((ImFont *)A->font_ui, T, x, y, TH->ink,
				"1. Crie uma chave gratuita no Google AI Studio.");
			y += T + (float)S(4);
			draw_text((ImFont *)A->font_ui, T, x, y, TH->ink,
				"2. Cole a chave acima e clique em Salvar.");
			y += T + (float)S(4);
			draw_text((ImFont *)A->font_ui, T, x, y, TH->ink,
				"3. Selecione um trecho ou uma página e peça um resumo.");

			{
				float bx = pos.x + width - pad;
				float save_w = button_width("SALVAR", 104);
				float cancel_w = button_width("CANCELAR", 104);
				float remove_w = button_width("REMOVER", 96);
				bx -= save_w;
				if (text_button("##ai_save", "SALVAR", bx, footer_y, save_w,
					btn_h, 1))
					app_ai_save_key(A);
				bx -= (float)S(8) + cancel_w;
				if (text_button("##ai_cancel", "CANCELAR", bx, footer_y, cancel_w,
					btn_h, 0))
					A->ai_open = 0;
				if (A->ai_key_set) {
					bx -= (float)S(8) + remove_w;
					if (text_button("##ai_remove", "REMOVER", bx, footer_y,
						remove_w, btn_h, 0))
						app_ai_remove_key(A);
				}
			}
			if (link_button("##ai_studio", "Obter chave no AI Studio", x,
				footer_y, button_width("Obter chave no AI Studio", 250), btn_h))
				open_url(L"https://aistudio.google.com/app/apikey");
		} else {
			float answer_h;
			int ask_enter = 0;

			y += header_advance;
			draw_text((ImFont *)A->font_small, T, x, y, TH->muted,
				"CONTEXTO ENVIADO");
			y += T + (float)S(4);
			draw_text((ImFont *)A->font_ui, T, x, y,
				A->doc.doc ? TH->ink : TH->muted, context);
			y += T + (float)S(18);

			draw_text((ImFont *)A->font_bold, T, x, y, TH->muted,
				"PERGUNTA");
			y += T + (float)S(8);
			igSetCursorScreenPos(v2(x, y));
			igSetNextItemWidth(field_w);
			igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, v2((float)S(10), (float)S(9)));
			igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, (float)S(9));
			igPushStyleColor_U32(ImGuiCol_FrameBg, TH->field);
			igPushStyleColor_U32(ImGuiCol_NavCursor, 0);
			in_h = igGetFrameHeight();
			if (A->ai_focus_question) {
				igSetKeyboardFocusHere(0);
				A->ai_focus_question = 0;
			}
			ask_enter = igInputTextWithHint("##ai_question",
				"Ex.: quais são as coberturas e os valores desta apólice?",
				A->ai_question, sizeof(A->ai_question),
				ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL) ? 1 : 0;
			igPopStyleColor(2);
			igPopStyleVar(2);
			stroke_round(x + 0.5f, y + 0.5f, x + field_w - 0.5f, y + in_h - 0.5f,
				(float)S(9), 1.0f, TH->border);
			if (ask_enter)
				app_ai_send(A, 0);
			y += in_h + (float)S(12);

			{
				float row_h = btn_h;
				const char *sum_label = A->has_selection ?
					"RESUMIR SELEÇÃO" : "RESUMIR PÁGINA";
				float ask_w = button_width("PERGUNTAR", 118);
				float sum_w = button_width(sum_label, 150);
				float edit_w = button_width("ALTERAR CHAVE", 130);
				float ask_x = pos.x + width - pad - ask_w;
				float sum_x = ask_x - (float)S(10) - sum_w;

				if (A->ai_busy) {
					draw_spinner(x + (float)S(9), y + row_h * 0.5f,
						8.0f * U, TH->accent);
					draw_text((ImFont *)A->font_ui, T, x + (float)S(26),
						y + (row_h - T) * 0.5f, TH->muted,
						"Consultando o Gemini…");
				} else {
					if (text_button("##ai_key_edit", "ALTERAR CHAVE", x, y,
						edit_w, row_h, 0)) {
						A->ai_setup = 1;
						A->ai_focus_key = 1;
						A->ai_key_input[0] = 0;
						free(A->ai_error);
						A->ai_error = NULL;
					}
					if (text_button("##ai_sum", sum_label, sum_x, y, sum_w,
						row_h, 0))
						app_ai_send(A, 1);
					if (text_button("##ai_ask", "PERGUNTAR", ask_x, y, ask_w,
						row_h, 1))
						app_ai_send(A, 0);
				}
			}
			y += btn_h + (float)S(14);

			draw_text((ImFont *)A->font_small, T, x, y, TH->muted,
				"RESPOSTA");
			y += T + (float)S(6);
			answer_h = footer_y - (float)S(14) - y;
			if (answer_h < (float)S(90))
				answer_h = (float)S(90);

			if (A->ai_busy) {
				fill_round(x, y, x + field_w, y + answer_h, (float)S(9), TH->field);
				stroke_round(x + 0.5f, y + 0.5f, x + field_w - 0.5f,
					y + answer_h - 0.5f, (float)S(9), 1.0f, TH->border);
				draw_spinner(x + field_w * 0.5f, y + answer_h * 0.5f - (float)S(12),
					9.0f * U, TH->accent);
				draw_text_center((ImFont *)A->font_ui, T,
					x + field_w * 0.5f, y + answer_h * 0.5f + (float)S(18),
					TH->muted, "Consultando o Gemini…");
			} else if (A->ai_answer) {
				igSetCursorScreenPos(v2(x, y));
				igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, v2((float)S(12), (float)S(10)));
				igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, (float)S(9));
				igPushStyleColor_U32(ImGuiCol_FrameBg, TH->field);
				igInputTextMultiline("##ai_answer", A->ai_answer,
					strlen(A->ai_answer) + 1, v2(field_w, answer_h),
					ImGuiInputTextFlags_ReadOnly, NULL, NULL);
				igPopStyleColor(1);
				igPopStyleVar(2);
				stroke_round(x + 0.5f, y + 0.5f, x + field_w - 0.5f,
					y + answer_h - 0.5f, (float)S(9), 1.0f, TH->border);
			} else if (A->ai_error) {
				fill_round(x, y, x + field_w, y + answer_h, (float)S(9), TH->field);
				stroke_round(x + 0.5f, y + 0.5f, x + field_w - 0.5f,
					y + answer_h - 0.5f, (float)S(9), 1.0f, TH->border);
				igPushFont((ImFont *)A->font_ui, T);
				igPushStyleColor_U32(ImGuiCol_Text, TH->error);
				igSetCursorScreenPos(v2(x + (float)S(14), y + (float)S(12)));
				igPushTextWrapPos(pad + field_w - (float)S(14));
				igTextUnformatted(A->ai_error, NULL);
				igPopTextWrapPos();
				igPopStyleColor(1);
				igPopFont();
			} else {
				fill_round(x, y, x + field_w, y + answer_h, (float)S(9), TH->field);
				stroke_round(x + 0.5f, y + 0.5f, x + field_w - 0.5f,
					y + answer_h - 0.5f, (float)S(9), 1.0f, TH->border);
				draw_text_center((ImFont *)A->font_ui, T,
					x + field_w * 0.5f, y + answer_h * 0.5f, TH->muted,
					A->doc.doc ? "A resposta do Gemini aparecerá aqui." :
					"Abra um documento para perguntar.");
			}

			{
				float close_w = button_width("FECHAR", 104);
				float copy_w = button_width("COPIAR", 112);
				float close_x = pos.x + width - pad - close_w;
				float actions_w = close_w + (float)S(10);
				if (A->ai_answer && A->ai_answer[0])
					actions_w += copy_w + (float)S(10);
				draw_text_ellipsis((ImFont *)A->font_small, T, x,
					footer_y + btn_h * 0.5f,
					field_w - actions_w - (float)S(12),
					alpha_color(TH->muted, 200),
					"Envio manual · só o texto do contexto é enviado");
				if (A->ai_answer && A->ai_answer[0]) {
					if (text_button("##ai_copy", "COPIAR",
						close_x - (float)S(10) - copy_w, footer_y,
						copy_w, btn_h, 0)) {
						igSetClipboardText(A->ai_answer);
						app_set_status(A, 0, L"Resposta do Gemini copiada");
					}
				}
				if (text_button("##ai_close", "FECHAR", close_x, footer_y,
					close_w, btn_h, 1))
					A->ai_open = 0;
			}
		}
	}
	igEnd();
	DL = saved;
	g_ai_was_open = 1;
}

static void draw_modals(void)
{
	ImGuiIO *io = igGetIO_Nil();
	ImGuiWindowFlags flags;
	ImDrawList *saved = DL;

	if (!A->password_pending && !A->ocr_popup && !g_about_open && !A->ai_open) {
		g_password_was_open = 0;
		g_ocr_was_open = 0;
		g_ai_was_open = 0;
		return;
	}
	if (!A->ai_open)
		g_ai_was_open = 0;

	igSetNextWindowPos(v2(0.0f, 0.0f), ImGuiCond_Always, v2(0.0f, 0.0f));
	igSetNextWindowSize(io->DisplaySize, ImGuiCond_Always);
	flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground;
	if (igBegin("##modal_dim", NULL, flags)) {
		DL = igGetWindowDrawList();
		igSetCursorScreenPos(v2(0.0f, 0.0f));
		igInvisibleButton("##modal_block", io->DisplaySize, ImGuiButtonFlags_MouseButtonLeft);
		fill_rect(0.0f, 0.0f, io->DisplaySize.x, io->DisplaySize.y, TH->dim);
		DL = saved;
	}
	igEnd();

	if (A->password_pending)
		draw_password_modal();
	else if (A->ocr_popup)
		draw_ocr_modal();
	else if (A->ai_open)
		draw_ai_modal();
	else if (g_about_open)
		draw_about_modal();
}

static void select_all_page(void)
{
	Doc *doc = &A->doc;
	fz_point a, b;
	int count;
	if (!doc->doc || !doc->has_text)
		return;
	a.x = doc->page_bounds.x0;
	a.y = doc->page_bounds.y0;
	b.x = doc->page_bounds.x1;
	b.y = doc->page_bounds.y1;
	count = doc_selection_quads(doc, a, b, A->sel_quads, MAX_SEL_QUADS);
	A->sel_a = a;
	A->sel_b = b;
	A->sel_count = count;
	A->has_selection = count > 0;
	A->selecting = 0;
	if (count > 0)
		app_set_status(A, 0, L"Página inteira selecionada");
}

void ui_show_about(void)
{
	g_about_open = 1;
}

void ui_show_splash(void)
{
	g_splash_force = 1;
}

void ui_show_settings(void)
{
	g_settings_open = 1;
	g_settings_age = 1;
}

static int modal_open(void)
{
	return A->password_pending || A->ocr_popup || g_about_open || A->ai_open;
}

static void handle_shortcuts(void)
{
	ImGuiIO *io = igGetIO_Nil();
	int ctrl = io->KeyCtrl ? 1 : 0;
	int shift = io->KeyShift ? 1 : 0;

	if (igIsKeyPressed_Bool(ImGuiKey_Escape, false)) {
		if (A->password_pending) {
			app_cancel_password(A);
			return;
		}
		if (A->ocr_popup) {
			close_ocr_modal();
			return;
		}
		if (A->ai_open) {
			A->ai_open = 0;
			return;
		}
		if (g_about_open) {
			g_about_open = 0;
			return;
		}
		if (g_settings_open) {
			g_settings_open = 0;
			return;
		}
		if (A->find_open) {
			app_find_close(A);
			snprintf(g_last_query, sizeof(g_last_query), "%s", A->query);
			return;
		}
		if (A->has_selection) {
			A->has_selection = 0;
			A->sel_count = 0;
			A->selecting = 0;
		}
	}
	if (modal_open() || io->WantTextInput)
		return;

	if (igIsKeyPressed_Bool(ImGuiKey_F1, false)) {
		g_about_open = 1;
		return;
	}

	if (ctrl && igIsKeyPressed_Bool(ImGuiKey_T, false)) {
		app_open_dialog(A);
		return;
	}
	if (ctrl && igIsKeyPressed_Bool(ImGuiKey_W, false)) {
		if (A->tab_count > 0)
			app_close_tab(A, A->active_tab);
		return;
	}
	if (ctrl && igIsKeyPressed_Bool(ImGuiKey_Tab, false)) {
		if (A->tab_count > 1)
			app_next_tab(A, shift ? -1 : 1);
		return;
	}
	if (ctrl && igIsKeyPressed_Bool(ImGuiKey_H, false)) {
		if (A->doc.doc)
			app_toggle_mark(A);
		return;
	}
	if (ctrl && igIsKeyPressed_Bool(ImGuiKey_B, false)) {
		if (A->doc.doc)
			app_toggle_sidebar(A);
		return;
	}
	if (ctrl && igIsKeyPressed_Bool(ImGuiKey_G, false)) {
		app_ai_open_modal(A);
		return;
	}
	if (ctrl && igIsKeyPressed_Bool(ImGuiKey_O, false)) {
		app_open_dialog(A);
		return;
	}
	if (ctrl && igIsKeyPressed_Bool(ImGuiKey_F, false)) {
		if (!A->find_open) {
			find_open_now();
		} else {
			A->focus_find = 1;
		}
		return;
	}
	if (ctrl && shift && igIsKeyPressed_Bool(ImGuiKey_D, false)) {
		theme_toggle();
		return;
	}
	if (ctrl && igIsKeyPressed_Bool(ImGuiKey_C, false)) {
		if (A->doc.doc)
			app_copy(A);
		return;
	}
	if (ctrl && igIsKeyPressed_Bool(ImGuiKey_A, false)) {
		if (A->doc.doc)
			select_all_page();
		return;
	}
	if (ctrl && igIsKeyPressed_Bool(ImGuiKey_0, false)) {
		if (A->doc.doc)
			app_set_zoom(A, 1.0f);
		return;
	}
	if (ctrl && igIsKeyPressed_Bool(ImGuiKey_1, false)) {
		if (A->doc.doc)
			app_fit(A, FIT_WIDTH);
		return;
	}
	if (ctrl && igIsKeyPressed_Bool(ImGuiKey_2, false)) {
		if (A->doc.doc)
			app_fit(A, FIT_PAGE);
		return;
	}
	if (ctrl && (igIsKeyPressed_Bool(ImGuiKey_Equal, false) ||
		igIsKeyPressed_Bool(ImGuiKey_KeypadAdd, false))) {
		if (A->doc.doc)
			app_zoom_by(A, 1.25f);
		return;
	}
	if (ctrl && (igIsKeyPressed_Bool(ImGuiKey_Minus, false) ||
		igIsKeyPressed_Bool(ImGuiKey_KeypadSubtract, false))) {
		if (A->doc.doc)
			app_zoom_by(A, 0.8f);
		return;
	}
	if (igIsKeyPressed_Bool(ImGuiKey_PageUp, false)) {
		if (A->doc.doc)
			app_step_page(A, -1);
		return;
	}
	if (igIsKeyPressed_Bool(ImGuiKey_PageDown, false)) {
		if (A->doc.doc)
			app_step_page(A, 1);
		return;
	}
	if (igIsKeyPressed_Bool(ImGuiKey_F3, false)) {
		if (A->doc.doc && A->hit_count > 0)
			app_find_step(A, shift ? -1 : 1);
		return;
	}
	if (igIsKeyPressed_Bool(ImGuiKey_Home, false) && A->doc.doc) {
		A->scroll_y = 0;
		return;
	}
	if (igIsKeyPressed_Bool(ImGuiKey_End, false) && A->doc.doc) {
		A->scroll_y = A->scroll_max;
		return;
	}
}

void ui_init(App *app)
{
	ImGuiIO *io;
	ImFontConfig cfg;
	static ImWchar ui_ranges[] = {
		0x0020, 0x00FF,
		0x2013, 0x2014,
		0x2018, 0x201D,
		0x2022, 0x2022,
		0x2026, 0x2026,
		0x20AC, 0x20AC,
		0
	};
	static ImWchar icon_ranges[] = { 0x0020, 0x00FF, 0xE700, 0xF8FF, 0 };
	wchar_t fonts_dir[900];
	char fonts_utf8[900];
	char path[1200];
	int need;

	A = app;
	U = app->ui > 0.0f ? app->ui : 1.0f;
	T = (float)app->text_size * U;

	igGetStyle()->FontSizeBase = T;
	igGetStyle()->_NextFrameFontSizeBase = T;
	igGetStyle()->FontScaleMain = 1.0f;
	igGetStyle()->FontScaleDpi = 1.0f;

	if (!GetWindowsDirectoryW(fonts_dir, 900))
		wcscpy(fonts_dir, L"C:\\Windows");
	wcsncat(fonts_dir, L"\\Fonts", 900 - wcslen(fonts_dir) - 1);
	need = WideCharToMultiByte(CP_UTF8, 0, fonts_dir, -1, fonts_utf8, 900, NULL, NULL);
	if (need <= 0)
		strcpy(fonts_utf8, "C:\\Windows\\Fonts");

	io = igGetIO_Nil();
	memset(&cfg, 0, sizeof(cfg));
	cfg.FontDataOwnedByAtlas = 1;
	cfg.ExtraSizeScale = 1.0f;
	cfg.GlyphMaxAdvanceX = FLT_MAX;
	cfg.RasterizerMultiply = 1.0f;
	cfg.RasterizerDensity = 1.0f;
	cfg.OversampleH = 2;
	cfg.OversampleV = 1;

	snprintf(path, sizeof(path), "%s\\segoeui.ttf", fonts_utf8);
	app->font_ui = ImFontAtlas_AddFontFromFileTTF(io->Fonts, path, 16.0f, &cfg, ui_ranges);
	snprintf(path, sizeof(path), "%s\\seguisb.ttf", fonts_utf8);
	app->font_bold = ImFontAtlas_AddFontFromFileTTF(io->Fonts, path, 16.0f, &cfg, ui_ranges);
	snprintf(path, sizeof(path), "%s\\segmdl2.ttf", fonts_utf8);
	app->font_icon = ImFontAtlas_AddFontFromFileTTF(io->Fonts, path, 20.0f, &cfg, icon_ranges);
	if (!app->font_ui)
		app->font_ui = ImFontAtlas_AddFontDefault(io->Fonts, NULL);
	if (!app->font_bold)
		app->font_bold = app->font_ui;
	if (!app->font_icon)
		app->font_icon = app->font_ui;
	app->font_small = app->font_ui;
	app->font_title = app->font_bold;
	app->font_big = app->font_bold;
	app->font_icon_small = app->font_icon;
	app->font_icon_big = app->font_icon;
	io->FontDefault = (ImFont *)app->font_ui;

	glyph_set(G.open, 0xE8E5);
	glyph_set(G.search, 0xE721);
	glyph_set(G.prev, 0xE76B);
	glyph_set(G.next, 0xE76C);
	glyph_set(G.zoom_out, 0xE71F);
	glyph_set(G.zoom_in, 0xE8A3);
	glyph_set(G.fit, 0xE9A6);
	glyph_set(G.rotate, 0xE7AD);
	glyph_set(G.copy, 0xE8C8);
	glyph_set(G.ocr, 0xE8B8);
	glyph_set(G.close, 0xE8BB);
	glyph_set(G.close_small, 0xE711);
	glyph_set(G.min, 0xE921);
	glyph_set(G.max, 0xE922);
	glyph_set(G.restore, 0xE923);
	glyph_set(G.doc, 0xE8A5);
	glyph_set(G.moon, 0xE708);
	glyph_set(G.sun, 0xE706);
	glyph_set(G.info, 0xE946);
	glyph_set(G.mark, 0xE7E6);
	glyph_set(G.plus, 0xE710);
	glyph_set(G.sparkle, 0xE8BD);
	glyph_set(G.sidebar, 0xE8A0);
	glyph_set(G.settings, 0xE713);
	glyph_set(G.export_txt, 0xE74E);

	TH = app->state.dark ? &THEME_DARK : &THEME_LIGHT;
	style_apply(app);
	snprintf(g_last_query, sizeof(g_last_query), "%s", app->query);
	g_ocr_seen = NULL;
	g_password_was_open = 0;
	g_ocr_was_open = 0;
}

void ui_shutdown(App *app)
{
	(void)app;
	if (app->ocr_utf8) {
		free(app->ocr_utf8);
		app->ocr_utf8 = NULL;
	}
	A = NULL;
	DL = NULL;
	TH = NULL;
}

static void draw_splash(float width, float height)
{
	static ULONGLONG started;
	ULONGLONG now;
	float elapsed;
	float fade;
	float progress;
	float cx;
	float cy;
	float logo;
	float logo_x;
	float logo_y;
	float title_cy;
	float sub_cy;
	float bar_w;
	float bar_x;
	float bar_y;
	char label[32];
	int dots;
	int a;
	ImU32 ink;
	ImU32 muted;
	ImU32 accent;
	ImU32 on_accent;
	ImU32 soft;
	ImU32 track;
	ImDrawList *saved;

	if (started == 0)
		started = GetTickCount64();
	now = GetTickCount64();
	elapsed = (float)(now - started) / 1000.0f;
	if (elapsed >= 0.82f)
		return;
	fade = elapsed < 0.58f ? 1.0f : 1.0f - (elapsed - 0.58f) / 0.24f;
	if (fade < 0.0f)
		fade = 0.0f;
	a = (int)lroundf(255.0f * fade);
	ink = alpha_color(TH->ink, a);
	muted = alpha_color(TH->muted, a);
	accent = alpha_color(TH->accent, a);
	on_accent = alpha_color(TH->on_accent, a);
	soft = alpha_color(TH->accent_soft, (int)((float)a * 0.8f));
	track = alpha_color(TH->border, a);

	cx = width * 0.5f;
	cy = height * 0.5f;
	logo = (float)S(74) * (1.0f + 0.012f * sinf(elapsed * 7.0f));
	logo_x = cx - logo * 0.5f;
	logo_y = cy - (float)S(82) - logo * 0.5f;

	saved = DL;
	DL = igGetForegroundDrawList_Nil();
	fill_rect(0.0f, 0.0f, width, height, alpha_color(TH->surface, a));
	fill_round(logo_x - (float)S(9), logo_y - (float)S(9), logo_x + logo + (float)S(9),
		logo_y + logo + (float)S(9), (float)S(24), soft);
	fill_round(logo_x, logo_y, logo_x + logo, logo_y + logo, (float)S(19), accent);
	draw_text_center((ImFont *)A->font_icon, logo * 0.52f, cx, logo_y + logo * 0.5f,
		on_accent, G.doc);
	title_cy = logo_y + logo + (float)S(30);
	draw_text_center((ImFont *)A->font_bold, T, cx, title_cy, ink, "PDF LEVE");
	sub_cy = title_cy + T + (float)S(12);
	dots = 1 + ((int)(elapsed * 3.0f) % 3);
	snprintf(label, sizeof(label), "CARREGANDO%s", dots == 1 ? "." : (dots == 2 ? ".." : "..."));
	draw_text_center((ImFont *)A->font_ui, T, cx, sub_cy, muted, label);

	bar_w = (float)S(150);
	bar_x = cx - bar_w * 0.5f;
	bar_y = sub_cy + T * 0.5f + (float)S(18);
	progress = elapsed / 0.52f;
	if (progress > 1.0f)
		progress = 1.0f;
	fill_round(bar_x, bar_y, bar_x + bar_w, bar_y + (float)S(3), (float)S(2), track);
	if (progress > 0.0f)
		fill_round(bar_x, bar_y, bar_x + bar_w * progress, bar_y + (float)S(3),
			(float)S(2), accent);
	DL = saved;
}

void ui_frame(App *app)
{
	ImGuiIO *io;
	ImGuiWindowFlags flags;
	ImVec2_c display;
	int title_h;
	int toolbar_h;
	int find_h;
	int status_h;
	float sidebar_w = 0.0f;
	float top;
	float width;
	float height;

	A = app;
	U = app->ui > 0.0f ? app->ui : 1.0f;
	T = (float)app->text_size * U;
	TH = app->state.dark ? &THEME_DARK : &THEME_LIGHT;
	if (g_styled_dark != app->state.dark || fabsf(g_styled_ui - U) > 0.001f ||
		g_styled_text != app->text_size)
		style_apply(app);

	io = igGetIO_Nil();
	display = io->DisplaySize;
	width = display.x;
	height = display.y;
	A->width = (int)lroundf(width);
	A->height = (int)lroundf(height);

	g_modal = modal_open();
	handle_shortcuts();
	/* A modal may have been closed by Esc above. */
	g_modal = modal_open();
	if (g_modal)
		g_settings_open = 0;

	title_h = S(44);
	toolbar_h = S(58);
	find_h = S(46);
	status_h = S(30);

	igSetNextWindowPos(v2(0.0f, 0.0f), ImGuiCond_Always, v2(0.0f, 0.0f));
	igSetNextWindowSize(v2(width, height), ImGuiCond_Always);
	flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
	igBegin("##pdf_leve_root", NULL, flags);
	DL = igGetWindowDrawList();

	fill_rect(0.0f, 0.0f, width, height, TH->chrome);

	if (A->tab_count > 0)
		draw_tabs(width, title_h + toolbar_h, S(38));
	if (A->tab_changed) {
		snprintf(g_last_query, sizeof(g_last_query), "%s", A->query);
		g_jump_guard = 0;
		g_ocr_seen = NULL;
		A->tab_changed = 0;
	}

	top = (float)title_h + (float)toolbar_h;
	if (A->tab_count > 0)
		top += (float)S(38);
	if (A->find_open)
		top += (float)find_h;
	if (A->doc.doc && A->sidebar_open)
		sidebar_w = (float)S(232);
	g_view = rectf(sidebar_w, top, width, height - (float)status_h);
	if (g_view.y1 < g_view.y0 + 1.0f)
		g_view.y1 = g_view.y0 + 1.0f;

	A->view_left = (int)g_view.x0;
	A->view_top = (int)g_view.y0;
	A->view_w = (int)(g_view.x1 - g_view.x0);
	A->view_h = (int)(g_view.y1 - g_view.y0);
	doc_set_viewport(&A->doc, A->view_w, A->view_h, A->dpi, A->ui);

	draw_canvas(g_view);
	if (sidebar_w > 0.0f)
		draw_sidebar(sidebar_w, top, height - (float)status_h);
	draw_titlebar(width, title_h);
	draw_toolbar(width, title_h, toolbar_h);
	if (A->find_open)
		draw_findbar(width, title_h + toolbar_h + (A->tab_count > 0 ? S(38) : 0),
			find_h);
	draw_statusbar(width, height - (float)status_h, status_h);
	draw_settings_popover(width, top + (float)S(6), height - (float)status_h);

	igEnd();

	if (A->find_open && A->search_pending &&
		igGetTime() >= A->search_pending_at) {
		A->search_pending = 0;
		find_apply_now();
	}
	if (A->find_open && !g_jump_guard && A->hit_index < 0 && A->hit_count > 0) {
		g_jump_guard = 1;
		app_find_step(A, 1);
	}

	if (g_modal)
		draw_modals();
	if ((!app->saving_disabled || g_splash_force) && !g_modal)
		draw_splash(width, height);

	igRender();

	{
		static wchar_t last_title[1200];
		wchar_t title[1200];
		const wchar_t *name = A->doc.doc && A->doc.path ? base_name_w(A->doc.path) : NULL;
		if (name)
			_snwprintf(title, 1199, L"%s — %s", APP_NAME, name);
		else
			_snwprintf(title, 1199, L"%s — %s", APP_NAME, APP_AUTHOR);
		title[1199] = 0;
		if (wcscmp(title, last_title) != 0) {
			wcsncpy(last_title, title, 1199);
			last_title[1199] = 0;
			SetWindowTextW(A->hwnd, title);
		}
	}
}

void ui_clear_color(const App *app, int *r, int *g, int *b)
{
	if (app->state.dark) {
		*r = 0x19;
		*g = 0x17;
		*b = 0x14;
	} else {
		*r = 0xD6;
		*g = 0xCF;
		*b = 0xC2;
	}
}

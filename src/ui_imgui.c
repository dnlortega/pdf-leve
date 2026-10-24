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
} Glyphs;

typedef struct UiRect {
	float x0, y0, x1, y1;
} UiRect;

static App *A;
static ImDrawList *DL;
static const UiTheme *TH;
static float U;
static Glyphs G;
static UiRect g_view;
static int g_modal;

static char g_last_query[256];
static int g_jump_guard;
static wchar_t *g_ocr_seen;
static int g_password_was_open;
static int g_ocr_was_open;
static int g_styled_dark = -1;
static float g_styled_ui = -1.0f;
static int g_about_open;

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

	style->_NextFrameFontSizeBase = 15.0f * U;
	style->FontSizeBase = 15.0f * U;
	style->FontScaleMain = 1.0f;
	style->FontScaleDpi = 1.0f;

	g_styled_dark = app->state.dark;
	g_styled_ui = U;
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
		igPushFont((ImFont *)A->font_small, 12.0f * U);
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
	draw_text_center((ImFont *)A->font_ui, 14.0f * U, x + w * 0.5f,
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
	draw_text_center((ImFont *)A->font_ui, 14.0f * U, x + w * 0.5f,
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
	float title_size = 15.0f * U;
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
		TH->ink, "PDF Leve");
	text_x += measure((ImFont *)A->font_bold, title_size, "PDF Leve").x + (float)S(12);

	if (A->doc.doc && A->doc.path) {
		draw_text((ImFont *)A->font_ui, 14.0f * U, text_x - (float)S(2),
			cy - 7.0f * U, TH->muted, "\xC2\xB7");
		text_x += (float)S(8);
		draw_text_ellipsis((ImFont *)A->font_ui, 14.0f * U, text_x, cy,
			min_x - (float)S(8) - text_x, TH->muted, u8w(base_name_w(A->doc.path)));
	} else {
		draw_text_ellipsis((ImFont *)A->font_ui, 14.0f * U, text_x, cy,
			min_x - (float)S(8) - text_x, TH->muted,
			"leitor de PDF  \xC2\xB7  por Daniel Ortega Pereira");
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
			draw_text_center((ImFont *)A->font_icon, 11.0f * U,
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
	float ic = 17.0f * U;
	float page_y = (float)top + (float)(toolbar_h - S(30)) / 2.0f;
	float page_x;
	float sep_x;
	float theme_x;
	float info_x;
	float ocr_x;
	float copy_x;
	float search_x;
	float rotate_x;
	float fit_x;
	float zin_x;
	float pill_x;
	float zout_x;
	float pill_w = (float)S(54);
	float pill_h = (float)S(28);
	int has_doc = A->doc.doc != NULL;
	char label[128];

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
	fill_round(page_x, page_y, page_x + (float)S(82), page_y + (float)S(30),
		(float)S(8), TH->surface);
	stroke_round(page_x + 0.5f, page_y + 0.5f, page_x + (float)S(82) - 0.5f,
		page_y + (float)S(30) - 0.5f, (float)S(8), 1.0f, TH->border);
	draw_text_center((ImFont *)A->font_bold, 13.0f * U, page_x + (float)S(41),
		page_y + (float)S(15), has_doc ? TH->ink : TH->muted, label);

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
	copy_x = ocr_x - (float)S(4) - (float)S(36);
	search_x = copy_x - (float)S(4) - (float)S(36);
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
		draw_text_center((ImFont *)A->font_bold, 13.0f * U, pill_x + pill_w * 0.5f,
			y + (float)S(4) + pill_h * 0.5f, has_doc ? (hovered ? TH->ink : TH->muted) :
			alpha_color(TH->muted, 120), label);
		if (igIsItemClicked(ImGuiMouseButton_Left) && has_doc)
			app_set_zoom(A, 1.0f);
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
}

static void draw_findbar(float width, int top, int find_h)
{
	ImGuiIO *io = igGetIO_Nil();
	float field_h = (float)S(30);
	float field_y = (float)top + ((float)find_h - field_h) * 0.5f;
	float close_x = width - (float)S(14) - (float)S(30);
	float next_x = close_x - (float)S(4) - (float)S(30);
	float prev_x = next_x - (float)S(4) - (float)S(30);
	float field_x = (float)S(14);
	float field_w = prev_x - (float)S(88) - field_x;
	float count_right = prev_x - (float)S(6);
	char info[160];
	int focused;

	fill_rect(0, (float)top, width, (float)(top + find_h), TH->chrome);
	fill_rect(0, (float)(top + find_h) - 1.0f, width, (float)(top + find_h), TH->border);

	fill_round(field_x, field_y, field_x + field_w, field_y + field_h, (float)S(8), TH->surface);
	draw_text_center((ImFont *)A->font_icon_small, 13.0f * U, field_x + (float)S(18),
		field_y + field_h * 0.5f, TH->muted, G.search);

	igSetCursorScreenPos(v2(field_x + (float)S(32), field_y));
	igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, v2((float)S(4), (float)S(7)));
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
		draw_text_right((ImFont *)A->font_small, 12.0f * U, count_right,
			(float)top + (float)find_h * 0.5f, TH->muted, info);
	}

	if (tool_button("##find_prev", G.prev, prev_x, field_y, (float)S(30), 13.0f * U,
		"Ocorrência anterior (Shift+F3)", A->hit_count > 0, 0, 0))
		app_find_step(A, -1);
	if (tool_button("##find_next", G.next, next_x, field_y, (float)S(30), 13.0f * U,
		"Próxima ocorrência (F3)", A->hit_count > 0, 0, 0))
		app_find_step(A, 1);
	if (tool_button("##find_close", G.close_small, close_x, field_y, (float)S(30), 13.0f * U,
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
	draw_text_center((ImFont *)A->font_bold, 30.0f * U, cx, start_y + icon +
		(float)S(18) + (float)S(17), TH->ink, "PDF Leve");
	draw_text_center((ImFont *)A->font_ui, 15.0f * U, cx,
		start_y + icon + (float)S(18) + (float)S(34) + (float)S(4) + (float)S(10),
		TH->muted, "Leve, rápido e sem distrações.");

	{
		const char *credit = "Desenvolvido por Daniel Ortega Pereira";
		ImFont *small = (ImFont *)A->font_small;
		float credit_size = 12.5f * U;
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
		draw_text_center((ImFont *)A->font_icon_big, 26.0f * U, cx,
			by + (float)S(27), TH->on_accent, G.open);
		if (hovered && igIsItemHovered(ImGuiHoveredFlags_DelayShort))
			tooltip_show("Abrir PDF (Ctrl+O)");
		if (clicked)
			app_open_dialog(A);
		draw_text_center((ImFont *)A->font_small, 12.0f * U, cx,
			by + (float)S(54) + (float)S(9), TH->muted,
			"Arraste um PDF aqui ou pressione Ctrl+O");
	}

	if (rows > 0) {
		float panel_top = start_y + content_h + (float)S(24);
		float panel_x0 = cx - panel_w * 0.5f;
		float panel_x1 = cx + panel_w * 0.5f;
		float panel_y0 = panel_top - (float)S(8);
		float panel_y1 = panel_top + rows * row_h + (float)S(8);
		float label_y = panel_y0 - (float)S(13);
		draw_text((ImFont *)A->font_small, 12.0f * U, panel_x0 + (float)S(14),
			label_y - 6.0f * U, TH->muted, "Recentes");
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
			float cy_name = row.y0 + (float)S(15);
			float cy_sub = row.y0 + (float)S(34);

			hovered = in_rect(row, mouse);
			if (hovered)
				fill_round(row.x0 + (float)S(4), row.y0 + (float)S(2), row.x1 - (float)S(4),
					row.y1 - (float)S(2), (float)S(8), TH->hover);
			fill_round(icon_x, icon_y, icon_x + icon_sz, icon_y + icon_sz, (float)S(8),
				TH->accent_soft);
			draw_text_center((ImFont *)A->font_icon, 16.0f * U, icon_x + icon_sz * 0.5f,
				icon_y + icon_sz * 0.5f, TH->accent, G.doc);
			wcsncpy(name, base_name_w(recent->path), 511);
			name[511] = 0;
			parent_folder_w(recent->path, folder, 512);
			_snwprintf(sub, 699, L"%s  \u00B7  página %d",
				folder[0] ? folder : L"Documento", recent->page + 1);
			sub[699] = 0;
			state_format_time(recent->used, when, 64);
			draw_text_ellipsis((ImFont *)A->font_bold, 14.0f * U, text_left, cy_name,
				row.x1 - (float)S(18) - time_w - text_left, TH->ink, u8w(name));
			draw_text_ellipsis((ImFont *)A->font_small, 12.0f * U, text_left, cy_sub,
				row.x1 - (float)S(18) - time_w - text_left, TH->muted, u8w(sub));
			draw_text_right((ImFont *)A->font_small, 12.0f * U, row.x1 - (float)S(14),
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

	if (!g_modal && igIsMouseHoveringRect(v2(page_area.x0, page_area.y0),
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

	if (!g_modal) {
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
			draw_text_center((ImFont *)A->font_ui, 15.0f * U, cx, cy, TH->muted,
				doc->error[0] ? u8w(doc->error) : "Renderizando\xE2\x80\xA6");
		}
	} else {
		draw_text_center((ImFont *)A->font_ui, 15.0f * U,
			(view.x0 + view.x1) * 0.5f, (view.y0 + view.y1) * 0.5f, TH->muted,
			doc->error[0] ? u8w(doc->error) : "Renderizando\xE2\x80\xA6");
	}

	draw_scrollbars(view);
}

static void draw_statusbar(float width, float top, int status_h)
{
	const char *left;
	char right[512];
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
	draw_text_ellipsis((ImFont *)A->font_small, 12.0f * U, (float)S(16), cy,
		width * 0.55f, left_color, left);

	if (!A->doc.doc)
		return;
	snprintf(right, sizeof(right), "Página %d de %d  ·  %d%%", A->doc.page + 1,
		A->doc.page_count, (int)lroundf(A->doc.zoom * 100.0f));
	if (A->doc.encrypted)
		strncat(right, "  ·  protegido", sizeof(right) - strlen(right) - 1);
	if (!A->doc.copy_allowed)
		strncat(right, "  ·  cópia restrita", sizeof(right) - strlen(right) - 1);
	MultiByteToWideChar(CP_UTF8, 0, right, -1, right_w, 512);
	right_w[511] = 0;
	draw_text_right((ImFont *)A->font_small, 12.0f * U, width - (float)S(16),
		cy, TH->muted, u8w(right_w));
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
		draw_text((ImFont *)A->font_bold, 19.0f * U, x + (float)S(42),
			y - 1.0f * U, TH->ink, "PDF protegido");
		draw_text((ImFont *)A->font_ui, 13.0f * U, x + (float)S(42),
			y + (float)S(23), TH->muted, "Este PDF exige uma senha de abertura.");

		y += (float)S(58);
		draw_text((ImFont *)A->font_bold, 12.0f * U, x, y, TH->muted, "Senha");
		y += (float)S(20);
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

		y += (float)S(42);
		if (A->password_error[0])
			draw_text_ellipsis((ImFont *)A->font_small, 12.0f * U, x, y + 7.0f * U,
				width - pad * 2.0f, TH->error, u8w(A->password_error));

		{
			float by = pos.y + height - pad - (float)S(36);
			float bx = pos.x + width - pad - (float)S(104);
			if (text_button("##pwd_open", "Abrir", bx, by, (float)S(104),
				(float)S(36), 1))
				app_submit_password(A);
			if (text_button("##pwd_cancel", "Cancelar", bx - (float)S(10) - (float)S(104),
				by, (float)S(104), (float)S(36), 0))
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
	field_w = width - pad * 2.0f;
	text_h = height - pad * 2.0f - (float)S(96);

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
		draw_text((ImFont *)A->font_bold, 19.0f * U, x + (float)S(42),
			y - 1.0f * U, TH->ink, "Texto reconhecido");
		snprintf(subtitle, sizeof(subtitle), "Página %d  ·  OCR local do Windows",
			A->ocr_page + 1);
		draw_text((ImFont *)A->font_ui, 12.0f * U, x + (float)S(42),
			y + (float)S(23), TH->muted, subtitle);

		y += (float)S(58);
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
			float by = pos.y + height - pad - (float)S(36);
			float bx = pos.x + width - pad - (float)S(150);
			if (A->ocr_focus_copied == 0) {
				igSetCursorScreenPos(v2(bx, by));
				igSetKeyboardFocusHere(0);
				A->ocr_focus_copied = 1;
			}
			if (text_button("##ocr_copy", "Copiar texto", bx, by,
				(float)S(150), (float)S(36), 1)) {
				igSetClipboardText(A->ocr_utf8 ? A->ocr_utf8 : "");
				app_set_status(A, 0, L"%s", L"Texto do OCR copiado");
				A->ocr_focus_copied = 2;
			}
			if (text_button("##ocr_close", "Fechar",
				bx - (float)S(10) - (float)S(104), by, (float)S(104),
				(float)S(36), 0))
				close_ocr_modal();
		}
	}
	igEnd();
	DL = saved;
	g_ocr_was_open = 1;
}

static void draw_about_modal(void)
{
	ImGuiIO *io = igGetIO_Nil();
	float width = (float)S(470);
	float height = (float)S(356);
	float cx = io->DisplaySize.x * 0.5f;
	float cy = io->DisplaySize.y * 0.5f;
	float pad = (float)S(26);
	float rounding = (float)S(13);
	float field_w = width - pad * 2.0f;
	float link_h = (float)S(40);
	ImDrawList *saved = DL;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;

	if (width > io->DisplaySize.x - (float)S(60))
		width = io->DisplaySize.x - (float)S(60);
	if (height > io->DisplaySize.y - (float)S(60))
		height = io->DisplaySize.y - (float)S(60);
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
		float x = pos.x + pad;
		float y = pos.y + pad;
		float footer_y = pos.y + height - pad - (float)S(36);

		fill_round(pos.x, pos.y, pos.x + width, pos.y + height, rounding, TH->surface);
		stroke_round(pos.x + 0.5f, pos.y + 0.5f, pos.x + width - 0.5f,
			pos.y + height - 0.5f, rounding, 1.0f, TH->border);
		draw_mark(x, y, (float)S(38));
		draw_text((ImFont *)A->font_bold, 21.0f * U, x + (float)S(50),
			y + 1.0f * U, TH->ink, "PDF Leve");
		draw_text_right((ImFont *)A->font_small, 12.0f * U, pos.x + width - pad,
			y + 10.0f * U, TH->muted, "versão 1.0");
		draw_text((ImFont *)A->font_ui, 13.0f * U, x + (float)S(50),
			y + (float)S(27), TH->muted, "Leitor de PDF leve, rápido e sem distrações.");

		y += (float)S(58);
		fill_rect(x, y, pos.x + width - pad, y + 1.0f, TH->border_soft);
		y += (float)S(18);
		draw_text((ImFont *)A->font_small, 11.0f * U, x, y, TH->muted,
			"DESENVOLVIDO POR");
		y += (float)S(20);
		draw_text((ImFont *)A->font_bold, 18.0f * U, x, y, TH->ink,
			"Daniel Ortega Pereira");

		y += (float)S(32);
		if (link_button("##about_linkedin",
			"LinkedIn   ·   linkedin.com/in/daniel-op", x, y, field_w, link_h))
			open_url(APP_LINKEDIN);
		y += link_h + (float)S(8);
		if (link_button("##about_whatsapp",
			"WhatsApp   ·   +55 (14) 98129-4913", x, y, field_w, link_h))
			open_url(APP_WHATSAPP);

		draw_text((ImFont *)A->font_small, 12.0f * U, x,
			footer_y + (float)S(12), alpha_color(TH->muted, 200),
			"© 2026 Daniel Ortega Pereira");
		if (text_button("##about_close", "Fechar",
			pos.x + width - pad - (float)S(104), footer_y, (float)S(104),
			(float)S(36), 1))
			g_about_open = 0;
	}
	igEnd();
	DL = saved;
}

static void draw_modals(void)
{
	ImGuiIO *io = igGetIO_Nil();
	ImGuiWindowFlags flags;
	ImDrawList *saved = DL;

	if (!A->password_pending && !A->ocr_popup && !g_about_open) {
		g_password_was_open = 0;
		g_ocr_was_open = 0;
		return;
	}

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

static int modal_open(void)
{
	return A->password_pending || A->ocr_popup || g_about_open;
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
		if (g_about_open) {
			g_about_open = 0;
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

	igGetStyle()->FontSizeBase = 15.0f * U;
	igGetStyle()->_NextFrameFontSizeBase = 15.0f * U;
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

void ui_frame(App *app)
{
	ImGuiIO *io;
	ImGuiWindowFlags flags;
	ImVec2_c display;
	int title_h;
	int toolbar_h;
	int find_h;
	int status_h;
	float top;
	float width;
	float height;

	A = app;
	U = app->ui > 0.0f ? app->ui : 1.0f;
	TH = app->state.dark ? &THEME_DARK : &THEME_LIGHT;
	if (g_styled_dark != app->state.dark || fabsf(g_styled_ui - U) > 0.001f)
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

	top = (float)title_h + (float)toolbar_h;
	if (A->find_open)
		top += (float)find_h;
	g_view = rectf(0.0f, top, width, height - (float)status_h);
	if (g_view.y1 < g_view.y0 + 1.0f)
		g_view.y1 = g_view.y0 + 1.0f;

	A->view_left = (int)g_view.x0;
	A->view_top = (int)g_view.y0;
	A->view_w = (int)(g_view.x1 - g_view.x0);
	A->view_h = (int)(g_view.y1 - g_view.y0);
	doc_set_viewport(&A->doc, A->view_w, A->view_h, A->dpi);

	draw_canvas(g_view);
	draw_titlebar(width, title_h);
	draw_toolbar(width, title_h, toolbar_h);
	if (A->find_open)
		draw_findbar(width, title_h + toolbar_h, find_h);
	draw_statusbar(width, height - (float)status_h, status_h);

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

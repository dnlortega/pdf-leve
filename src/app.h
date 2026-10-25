#ifndef PDF_LEVE_APP_H
#define PDF_LEVE_APP_H

#include <windows.h>

#include "doc.h"
#include "gemini.h"
#include "ocr.h"
#include "state.h"

#define APP_NAME L"PDF Leve"
#define APP_AUTHOR L"Daniel Ortega Pereira"
#define APP_LINKEDIN L"https://www.linkedin.com/in/daniel-op/"
#define APP_WHATSAPP L"https://wa.me/5514981294913"
#define APP_CLASS L"PdfLeveImGuiMain"

#define MAX_HITS 1500
#define MAX_SEL_QUADS 768
#define MAX_TABS 10
#define MAX_MARKS 32
#define MAX_MARK_QUADS 128
#define THUMB_SLOTS 24
#define MAX_OUTLINE_ITEMS 256

typedef struct ThumbEntry {
	int page;
	int w;
	int h;
	unsigned long long used;
} ThumbEntry;

typedef struct Hit {
	int page;
	fz_quad quad;
} Hit;

typedef struct Mark {
	int page;
	int quad_count;
	fz_quad quads[MAX_MARK_QUADS];
} Mark;

typedef struct Tab {
	int id;
	Doc doc;

	int find_open;
	char query[256];
	char query_applied[256];
	Hit hits[MAX_HITS];
	int hit_count;
	int hit_index;
	int searching;
	int search_cursor;
	int search_pending;
	double search_pending_at;
	int focus_find;
	int open_find_focus;

	int has_selection;
	int selecting;
	fz_point sel_a;
	fz_point sel_b;
	fz_quad sel_quads[MAX_SEL_QUADS];
	int sel_count;

	int mark_count;
	Mark marks[MAX_MARKS];

	int scroll_y;
	int scroll_x;
	int scroll_max;
	int scroll_max_x;
	RECT rc_page;
	int view_left;
	int view_top;
	int view_w;
	int view_h;

	unsigned long long tex_id;
	int tex_page;
	int tex_rot;
	float tex_zoom;
	int tex_w;
	int tex_h;
	int render_pending;
	int last_view_w;
	int last_view_h;

	wchar_t *ocr_text;
	int ocr_page;
	wchar_t ocr_cache[1024];
	wchar_t ocr_source[1024];
	int ocr_popup;
	int ocr_popup_shown;
	int ocr_focus_copied;
} Tab;

typedef struct App {
	HINSTANCE inst;
	HWND hwnd;
	Doc doc;
	State state;

	int dpi;
	float ui;
	int ui_scale_pct;
	int text_size;
	int width;
	int height;

	void *font_ui;
	void *font_bold;
	void *font_small;
	void *font_title;
	void *font_big;
	void *font_icon;
	void *font_icon_small;
	void *font_icon_big;

	int find_open;
	char query[256];
	char query_applied[256];
	Hit hits[MAX_HITS];
	int hit_count;
	int hit_index;
	int searching;
	int search_cursor;
	int search_pending;
	double search_pending_at;
	int focus_find;
	int open_find_focus;

	int has_selection;
	int selecting;
	fz_point sel_a;
	fz_point sel_b;
	fz_quad sel_quads[MAX_SEL_QUADS];
	int sel_count;

	int scroll_y;
	int scroll_x;
	int scroll_max;
	int scroll_max_x;
	RECT rc_page;
	int view_left;
	int view_top;
	int view_w;
	int view_h;

	unsigned long long tex_id;
	int tex_page;
	int tex_rot;
	float tex_zoom;
	int tex_w;
	int tex_h;
	int render_pending;
	int last_view_w;
	int last_view_h;

	int mark_count;
	Mark marks[MAX_MARKS];

	int sidebar_open;
	int sidebar_tab;
	int sb_scroll;
	int sb_scroll_max;
	int sb_follow;
	ThumbEntry thumbs[THUMB_SLOTS];
	unsigned long long thumb_clock;

	DocOutlineItem outline[MAX_OUTLINE_ITEMS];
	int outline_count;
	int outline_loaded;

	volatile LONG ocr_busy;
	wchar_t *ocr_text;
	char *ocr_utf8;
	int ocr_page;
	wchar_t ocr_cache[1024];
	wchar_t ocr_source[1024];
	int ocr_popup;
	int ocr_popup_shown;
	int ocr_focus_copied;
	int ocr_owner_id;

	Tab tabs[MAX_TABS];
	int tab_count;
	int active_tab;
	int next_tab_id;
	int tab_changed;
	Doc pending_doc;
	int pending_active;

	char ai_key[GEMINI_KEY_MAX];
	char ai_key_input[GEMINI_KEY_MAX];
	int ai_key_set;
	int ai_open;
	int ai_setup;
	int ai_focus_key;
	int ai_focus_question;
	volatile LONG ai_busy;
	char ai_question[512];
	char *ai_answer;
	char *ai_error;
	char ai_context[96];
	int ai_owner_id;

	wchar_t status[256];
	int status_accent;
	double status_until;

	int password_pending;
	int password_popup;
	int password_focus;
	wchar_t password_path[1024];
	char password[128];
	wchar_t password_error[160];

	int closing;
	int saving_disabled;
	int frame_count;
} App;

void app_init(App *app, HINSTANCE inst, HWND hwnd);
void app_shutdown(App *app);

void app_open_dialog(App *app);
void app_open_path(App *app, const wchar_t *path);
void app_load_document(App *app, const wchar_t *path, const char *password);
void app_submit_password(App *app);
void app_cancel_password(App *app);

void app_switch_tab(App *app, int index);
void app_close_tab(App *app, int index);
void app_next_tab(App *app, int delta);
void app_toggle_mark(App *app);
void app_save_marks(App *app);
int app_has_mark_on_page(const App *app, int page);
void app_toggle_sidebar(App *app);
void app_thumbs_reset(App *app);
void app_set_ui_scale(App *app, int percent);
void app_set_text_size(App *app, int size);

void app_goto_page(App *app, int page);
void app_step_page(App *app, int delta);
void app_zoom_by(App *app, float factor);
void app_set_zoom(App *app, float zoom);
void app_fit(App *app, int mode);
void app_rotate(App *app);
void app_render_tick(App *app);
void app_mark_render(App *app);

void app_copy(App *app);
char *app_selection_utf8_dup(App *app);
void app_export_page_text(App *app);
void app_export_ocr_text(App *app);

void app_start_ocr(App *app);
void app_ocr_done(App *app, OcrResult *result);

void app_ai_open_modal(App *app);
void app_ai_save_key(App *app);
void app_ai_remove_key(App *app);
void app_ai_send(App *app, int summarize);
void app_ai_done(App *app, GeminiResult *result);
void app_ai_context_label(const App *app, char *out, size_t cap);

void app_find_restart(App *app);
void app_search_tick(App *app);
void app_find_step(App *app, int delta);
void app_find_close(App *app);

void app_set_status(App *app, int accent, const wchar_t *fmt, ...);
void app_save_state(App *app);
void app_remember(App *app);
long app_status_seconds(const App *app);

#endif

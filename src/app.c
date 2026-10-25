#include "app.h"
#include "gfx.h"
#include "mu.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <commdlg.h>

static double now_seconds(void)
{
	return (double)GetTickCount64() / 1000.0;
}

static float clamp_zoom(float zoom)
{
	if (zoom < MIN_ZOOM)
		return MIN_ZOOM;
	if (zoom > MAX_ZOOM)
		return MAX_ZOOM;
	return zoom;
}

static const wchar_t *base_name(const wchar_t *path)
{
	const wchar_t *slash;
	const wchar_t *name;
	if (!path)
		return L"";
	slash = wcsrchr(path, L'\\');
	name = slash ? slash + 1 : path;
	return name[0] ? name : path;
}

void app_set_status(App *app, int accent, const wchar_t *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	_vsnwprintf(app->status, 255, fmt, ap);
	va_end(ap);
	app->status[255] = 0;
	app->status_accent = accent;
	app->status_until = now_seconds() + 6.0;
}

long app_status_seconds(const App *app)
{
	double left = app->status_until - now_seconds();
	if (left <= 0.0)
		return 0;
	return (long)ceil(left);
}

void app_init(App *app, HINSTANCE inst, HWND hwnd)
{
	RECT rc;
	memset(app, 0, sizeof(*app));
	app->inst = inst;
	app->hwnd = hwnd;
	app->dpi = (int)GetDpiForWindow(hwnd);
	if (app->dpi <= 0)
		app->dpi = 96;
	app->ui_scale_pct = 100;
	app->ui = (float)app->dpi / 96.0f;
	GetClientRect(hwnd, &rc);
	app->width = rc.right - rc.left;
	app->height = rc.bottom - rc.top;
	app->view_w = app->width;
	app->view_h = app->height;
	doc_init(&app->doc);
	doc_set_viewport(&app->doc, app->view_w, app->view_h, app->dpi, app->ui);
	doc_init(&app->pending_doc);
	state_load(&app->state);
	if (app->state.ui_scale == 110 || app->state.ui_scale == 125)
		app->ui_scale_pct = app->state.ui_scale;
	app->ui = (float)app->dpi / 96.0f * (float)app->ui_scale_pct / 100.0f;
	app->text_size = app->state.text_size;
	app->sidebar_open = app->state.sidebar;
	app_thumbs_reset(app);
	app->hit_index = -1;
	app->tex_page = -1;
	app->ocr_page = -1;
	app->active_tab = -1;
	app->next_tab_id = 1;
	if (gemini_load_key(app->ai_key, sizeof(app->ai_key)) > 0)
		app->ai_key_set = 1;
}

void app_shutdown(App *app)
{
	int i;
	app_save_state(app);
	if (app->active_tab >= 0 && app->active_tab < app->tab_count) {
		Tab *active = &app->tabs[app->active_tab];
		if (app->doc.doc) {
			active->doc = app->doc;
			memset(&app->doc, 0, sizeof(Doc));
		}
		active->ocr_text = app->ocr_text;
		app->ocr_text = NULL;
	}
	free(app->ocr_text);
	app->ocr_text = NULL;
	free(app->ocr_utf8);
	app->ocr_utf8 = NULL;
	for (i = 0; i < app->tab_count; i++) {
		doc_shutdown(&app->tabs[i].doc);
		free(app->tabs[i].ocr_text);
		app->tabs[i].ocr_text = NULL;
	}
	doc_shutdown(&app->doc);
	if (app->pending_active)
		doc_shutdown(&app->pending_doc);
	free(app->ai_answer);
	app->ai_answer = NULL;
	free(app->ai_error);
	app->ai_error = NULL;
	SecureZeroMemory(app->ai_key, sizeof(app->ai_key));
	SecureZeroMemory(app->ai_key_input, sizeof(app->ai_key_input));
}

static void copy_to_clipboard(App *app, const wchar_t *text)
{
	size_t bytes;
	HGLOBAL mem;
	void *dest;
	if (!text || !text[0])
		return;
	bytes = (wcslen(text) + 1) * sizeof(wchar_t);
	mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
	if (!mem)
		return;
	dest = GlobalLock(mem);
	if (!dest) {
		GlobalFree(mem);
		return;
	}
	memcpy(dest, text, bytes);
	GlobalUnlock(mem);
	if (!OpenClipboard(app->hwnd)) {
		GlobalFree(mem);
		return;
	}
	EmptyClipboard();
	if (!SetClipboardData(CF_UNICODETEXT, mem))
		GlobalFree(mem);
	CloseClipboard();
}

void app_remember(App *app)
{
	Doc *doc = &app->doc;
	if (!doc->doc || !doc->path)
		return;
	state_touch(&app->state, doc->path, doc->page, doc->zoom, doc->fit, doc->rotation);
	state_set_last(&app->state, doc->path, doc->page, doc->zoom, doc->fit, doc->rotation);
}

void app_save_state(App *app)
{
	app->state.sidebar = app->sidebar_open;
	app->state.ui_scale = app->ui_scale_pct;
	if (app->saving_disabled)
		return;
	app_remember(app);
	state_save(&app->state);
}

static void pending_release(App *app)
{
	if (!app->pending_active)
		return;
	doc_shutdown(&app->pending_doc);
	memset(&app->pending_doc, 0, sizeof(Doc));
	app->pending_active = 0;
}

static void tab_store_live(App *app, Tab *tab)
{
	tab->doc = app->doc;
	memset(&app->doc, 0, sizeof(Doc));
	doc_invalidate(&tab->doc);

	tab->find_open = app->find_open;
	memcpy(tab->query, app->query, sizeof(tab->query));
	memcpy(tab->query_applied, app->query_applied, sizeof(tab->query_applied));
	memcpy(tab->hits, app->hits, sizeof(app->hits));
	tab->hit_count = app->hit_count;
	tab->hit_index = app->hit_index;
	tab->searching = app->searching;
	tab->search_cursor = app->search_cursor;
	tab->search_pending = app->search_pending;
	tab->search_pending_at = app->search_pending_at;
	tab->focus_find = app->focus_find;
	tab->open_find_focus = app->open_find_focus;

	tab->has_selection = app->has_selection;
	tab->selecting = app->selecting;
	tab->sel_a = app->sel_a;
	tab->sel_b = app->sel_b;
	memcpy(tab->sel_quads, app->sel_quads, sizeof(app->sel_quads));
	tab->sel_count = app->sel_count;

	tab->mark_count = app->mark_count;
	memcpy(tab->marks, app->marks, sizeof(app->marks));

	tab->scroll_y = app->scroll_y;
	tab->scroll_x = app->scroll_x;
	tab->scroll_max = app->scroll_max;
	tab->scroll_max_x = app->scroll_max_x;
	tab->rc_page = app->rc_page;
	tab->view_left = app->view_left;
	tab->view_top = app->view_top;
	tab->view_w = app->view_w;
	tab->view_h = app->view_h;

	tab->tex_id = app->tex_id;
	tab->tex_page = app->tex_page;
	tab->tex_rot = app->tex_rot;
	tab->tex_zoom = app->tex_zoom;
	tab->tex_w = app->tex_w;
	tab->tex_h = app->tex_h;
	tab->render_pending = app->render_pending;
	tab->last_view_w = app->last_view_w;
	tab->last_view_h = app->last_view_h;

	tab->ocr_text = app->ocr_text;
	app->ocr_text = NULL;
	tab->ocr_page = app->ocr_page;
	memcpy(tab->ocr_cache, app->ocr_cache, sizeof(tab->ocr_cache));
	memcpy(tab->ocr_source, app->ocr_source, sizeof(tab->ocr_source));
	tab->ocr_popup = app->ocr_popup;
	tab->ocr_popup_shown = app->ocr_popup_shown;
	tab->ocr_focus_copied = app->ocr_focus_copied;
}

static void tab_load_live(App *app, Tab *tab)
{
	app->doc = tab->doc;
	memset(&tab->doc, 0, sizeof(Doc));

	app->find_open = tab->find_open;
	memcpy(app->query, tab->query, sizeof(app->query));
	memcpy(app->query_applied, tab->query_applied, sizeof(app->query_applied));
	memcpy(app->hits, tab->hits, sizeof(app->hits));
	app->hit_count = tab->hit_count;
	app->hit_index = tab->hit_index;
	app->searching = tab->searching;
	app->search_cursor = tab->search_cursor;
	app->search_pending = tab->search_pending;
	app->search_pending_at = tab->search_pending_at;
	app->focus_find = tab->focus_find;
	app->open_find_focus = tab->open_find_focus;

	app->has_selection = tab->has_selection;
	app->selecting = tab->selecting;
	app->sel_a = tab->sel_a;
	app->sel_b = tab->sel_b;
	memcpy(app->sel_quads, tab->sel_quads, sizeof(app->sel_quads));
	app->sel_count = tab->sel_count;

	app->mark_count = tab->mark_count;
	memcpy(app->marks, tab->marks, sizeof(app->marks));

	app->scroll_y = tab->scroll_y;
	app->scroll_x = tab->scroll_x;
	app->scroll_max = tab->scroll_max;
	app->scroll_max_x = tab->scroll_max_x;
	app->rc_page = tab->rc_page;
	app->view_left = tab->view_left;
	app->view_top = tab->view_top;
	app->view_w = tab->view_w;
	app->view_h = tab->view_h;

	app->tex_id = 0;
	app->tex_page = -1;
	app->tex_rot = tab->tex_rot;
	app->tex_zoom = tab->tex_zoom;
	app->tex_w = tab->tex_w;
	app->tex_h = tab->tex_h;
	app->render_pending = 1;
	app->last_view_w = 0;
	app->last_view_h = 0;

	free(app->ocr_utf8);
	app->ocr_utf8 = NULL;
	app->ocr_text = tab->ocr_text;
	tab->ocr_text = NULL;
	app->ocr_page = tab->ocr_page;
	memcpy(app->ocr_cache, tab->ocr_cache, sizeof(app->ocr_cache));
	memcpy(app->ocr_source, tab->ocr_source, sizeof(app->ocr_source));
	app->ocr_popup = tab->ocr_popup;
	app->ocr_popup_shown = 0;
	app->ocr_focus_copied = tab->ocr_focus_copied;
	app->tab_changed = 1;
	app_thumbs_reset(app);
}

static void ai_clear_result(App *app)
{
	free(app->ai_answer);
	app->ai_answer = NULL;
	free(app->ai_error);
	app->ai_error = NULL;
	app->ai_context[0] = 0;
	app->ai_owner_id = 0;
}

void app_thumbs_reset(App *app)
{
	int i;
	for (i = 0; i < THUMB_SLOTS; i++) {
		app->thumbs[i].page = -1;
		app->thumbs[i].w = 0;
		app->thumbs[i].h = 0;
		app->thumbs[i].used = 0;
	}
	app->thumb_clock = 0;
	app->outline_count = 0;
	app->outline_loaded = 0;
	app->sb_scroll = 0;
	app->sb_scroll_max = 0;
}

void app_toggle_sidebar(App *app)
{
	if (!app->doc.doc)
		return;
	app->sidebar_open = !app->sidebar_open;
	if (app->sidebar_open)
		app->sb_follow = 1;
	app_save_state(app);
	app_set_status(app, 0, L"%s", app->sidebar_open ?
		L"Painel de navegação aberto" : L"Painel de navegação fechado");
}

void app_set_ui_scale(App *app, int percent)
{
	if (percent != 100 && percent != 110 && percent != 125)
		percent = 100;
	if (app->ui_scale_pct == percent)
		return;
	app->ui_scale_pct = percent;
	app->state.ui_scale = percent;
	app->ui = (float)app->dpi / 96.0f * (float)percent / 100.0f;
	app->render_pending = 1;
}

void app_set_text_size(App *app, int size)
{
	if (size != 15 && size != 17 && size != 19 && size != 21)
		size = 17;
	if (app->text_size == size)
		return;
	app->text_size = size;
	app->state.text_size = size;
	app_save_state(app);
}

static void live_reset(App *app)
{
	app->find_open = 0;
	app->query[0] = 0;
	app->query_applied[0] = 0;
	app->hit_count = 0;
	app->hit_index = -1;
	app->searching = 0;
	app->search_cursor = 0;
	app->search_pending = 0;
	app->focus_find = 0;
	app->open_find_focus = 0;

	app->has_selection = 0;
	app->selecting = 0;
	memset(&app->sel_a, 0, sizeof(app->sel_a));
	memset(&app->sel_b, 0, sizeof(app->sel_b));
	app->sel_count = 0;
	app->mark_count = 0;

	app->scroll_x = 0;
	app->scroll_y = 0;
	app->scroll_max = 0;
	app->scroll_max_x = 0;
	SetRectEmpty(&app->rc_page);

	app->tex_id = 0;
	app->tex_page = -1;
	app->tex_rot = 0;
	app->tex_zoom = 0.0f;
	app->tex_w = 0;
	app->tex_h = 0;
	app->render_pending = 1;
	app->last_view_w = 0;
	app->last_view_h = 0;

	free(app->ocr_text);
	app->ocr_text = NULL;
	free(app->ocr_utf8);
	app->ocr_utf8 = NULL;
	app->ocr_page = -1;
	app->ocr_cache[0] = 0;
	app->ocr_source[0] = 0;
	app->ocr_popup = 0;
	app->ocr_popup_shown = 0;
	app->ocr_focus_copied = 0;
	app_thumbs_reset(app);
}

static int tab_find_path(App *app, const wchar_t *path)
{
	int i;
	if (!path)
		return -1;
	if (app->active_tab >= 0 && app->doc.path &&
		_wcsicmp(app->doc.path, path) == 0)
		return app->active_tab;
	for (i = 0; i < app->tab_count; i++) {
		if (i == app->active_tab)
			continue;
		if (app->tabs[i].doc.path && _wcsicmp(app->tabs[i].doc.path, path) == 0)
			return i;
	}
	return -1;
}

static Tab *tab_by_id(App *app, int id)
{
	int i;
	for (i = 0; i < app->tab_count; i++) {
		if (app->tabs[i].id == id)
			return &app->tabs[i];
	}
	return NULL;
}

static void tab_remove_entry(App *app, int index)
{
	int i;
	if (index < 0 || index >= app->tab_count)
		return;
	for (i = index; i + 1 < app->tab_count; i++)
		app->tabs[i] = app->tabs[i + 1];
	app->tab_count--;
	memset(&app->tabs[app->tab_count], 0, sizeof(Tab));
}

static void capture_active(App *app)
{
	if (app->active_tab < 0 || app->active_tab >= app->tab_count)
		return;
	if (!app->doc.doc)
		return;
	tab_store_live(app, &app->tabs[app->active_tab]);
}

static void marks_dir(wchar_t *out, size_t cap)
{
	state_data_dir(out, cap);
	if (wcslen(out) + 7 < cap)
		wcscat(out, L"\\marks");
	CreateDirectoryW(out, NULL);
}

static unsigned long long marks_hash(const wchar_t *path)
{
	wchar_t lower[1024];
	unsigned long long hash = 1469598103934665603ULL;
	int i;
	if (!LCMapStringW(LOCALE_INVARIANT, LCMAP_LOWERCASE, path, -1, lower, 1024)) {
		wcsncpy(lower, path, 1023);
		lower[1023] = 0;
	}
	for (i = 0; lower[i]; i++) {
		hash ^= (unsigned long long)(unsigned short)lower[i];
		hash *= 1099511628211ULL;
	}
	return hash;
}

static void marks_path_for(const wchar_t *doc_path, wchar_t *out, size_t cap)
{
	wchar_t dir[1024];
	marks_dir(dir, 1024);
	_snwprintf(out, cap - 1, L"%s\\%016llx.bin", dir, marks_hash(doc_path));
	out[cap - 1] = 0;
}

void app_save_marks(App *app)
{
	unsigned int magic = 0x314B524Du;
	unsigned int version = 1u;
	unsigned int chars;
	unsigned int count;
	wchar_t path[1024];
	wchar_t temp[1024];
	FILE *file;
	int i;

	if (!app->doc.doc || !app->doc.path)
		return;
	marks_path_for(app->doc.path, path, 1024);
	if (app->mark_count <= 0) {
		DeleteFileW(path);
		return;
	}
	wcscpy(temp, path);
	wcscat(temp, L".tmp");
	file = _wfopen(temp, L"wb");
	if (!file)
		return;
	chars = (unsigned int)wcslen(app->doc.path);
	count = (unsigned int)app->mark_count;
	fwrite(&magic, sizeof(magic), 1, file);
	fwrite(&version, sizeof(version), 1, file);
	fwrite(&chars, sizeof(chars), 1, file);
	fwrite(&count, sizeof(count), 1, file);
	fwrite(app->doc.path, sizeof(wchar_t), chars, file);
	for (i = 0; i < app->mark_count; i++) {
		Mark *mark = &app->marks[i];
		unsigned int quads = (unsigned int)mark->quad_count;
		fwrite(&mark->page, sizeof(int), 1, file);
		fwrite(&quads, sizeof(quads), 1, file);
		fwrite(mark->quads, sizeof(fz_quad), quads, file);
	}
	fclose(file);
	MoveFileExW(temp, path, MOVEFILE_REPLACE_EXISTING);
}

static void marks_load(App *app)
{
	unsigned int magic = 0;
	unsigned int version = 0;
	unsigned int chars = 0;
	unsigned int count = 0;
	wchar_t stored[1024];
	wchar_t path[1024];
	FILE *file;
	int i;

	app->mark_count = 0;
	if (!app->doc.path)
		return;
	marks_path_for(app->doc.path, path, 1024);
	file = _wfopen(path, L"rb");
	if (!file)
		return;
	if (fread(&magic, sizeof(magic), 1, file) != 1 ||
		fread(&version, sizeof(version), 1, file) != 1 ||
		fread(&chars, sizeof(chars), 1, file) != 1 ||
		fread(&count, sizeof(count), 1, file) != 1 ||
		magic != 0x314B524Du || version != 1u ||
		chars == 0 || chars > 1023 || count > MAX_MARKS) {
		fclose(file);
		return;
	}
	if (fread(stored, sizeof(wchar_t), chars, file) != chars) {
		fclose(file);
		return;
	}
	stored[chars] = 0;
	if (_wcsicmp(stored, app->doc.path) != 0) {
		fclose(file);
		return;
	}
	for (i = 0; i < (int)count; i++) {
		int page = 0;
		unsigned int quads = 0;
		Mark *mark = &app->marks[app->mark_count];
		if (fread(&page, sizeof(page), 1, file) != 1 ||
			fread(&quads, sizeof(quads), 1, file) != 1)
			break;
		if (quads > MAX_MARK_QUADS || page < 0 || page >= app->doc.page_count)
			break;
		if (fread(mark->quads, sizeof(fz_quad), quads, file) != quads)
			break;
		mark->page = page;
		mark->quad_count = (int)quads;
		app->mark_count++;
	}
	fclose(file);
}

static int quads_overlap(fz_quad a, fz_quad b)
{
	float ax0 = fminf(fminf(a.ul.x, a.ur.x), fminf(a.ll.x, a.lr.x));
	float ax1 = fmaxf(fmaxf(a.ul.x, a.ur.x), fmaxf(a.ll.x, a.lr.x));
	float ay0 = fminf(fminf(a.ul.y, a.ur.y), fminf(a.ll.y, a.lr.y));
	float ay1 = fmaxf(fmaxf(a.ul.y, a.ur.y), fmaxf(a.ll.y, a.lr.y));
	float bx0 = fminf(fminf(b.ul.x, b.ur.x), fminf(b.ll.x, b.lr.x));
	float bx1 = fmaxf(fmaxf(b.ul.x, b.ur.x), fmaxf(b.ll.x, b.lr.x));
	float by0 = fminf(fminf(b.ul.y, b.ur.y), fminf(b.ll.y, b.lr.y));
	float by1 = fmaxf(fmaxf(b.ul.y, b.ur.y), fmaxf(b.ll.y, b.lr.y));
	return ax0 < bx1 && bx0 < ax1 && ay0 < by1 && by0 < ay1;
}

int app_has_mark_on_page(const App *app, int page)
{
	int i;
	for (i = 0; i < app->mark_count; i++) {
		if (app->marks[i].page == page)
			return 1;
	}
	return 0;
}

void app_toggle_mark(App *app)
{
	int page;
	int removed = 0;
	int i, j, k;

	if (!app->doc.doc)
		return;
	if (!app->has_selection || app->sel_count < 1) {
		app_set_status(app, 1, L"Selecione um trecho de texto para marcar.");
		return;
	}
	page = app->doc.page;
	for (i = 0; i < app->mark_count; i++) {
		Mark *mark = &app->marks[i];
		int overlaps = 0;
		if (mark->page == page) {
			for (j = 0; j < mark->quad_count && !overlaps; j++) {
				for (k = 0; k < app->sel_count; k++) {
					if (quads_overlap(mark->quads[j], app->sel_quads[k])) {
						overlaps = 1;
						break;
					}
				}
			}
		}
		if (overlaps)
			removed++;
		else if (removed > 0)
			app->marks[i - removed] = app->marks[i];
	}
	if (removed > 0) {
		app->mark_count -= removed;
		app_save_marks(app);
		app_set_status(app, 0, L"Marcação removida (%d restantes)", app->mark_count);
		return;
	}
	if (app->mark_count >= MAX_MARKS) {
		app_set_status(app, 1, L"Limite de %d marcações por documento.", MAX_MARKS);
		return;
	}
	{
		Mark *mark = &app->marks[app->mark_count];
		mark->page = page;
		mark->quad_count = app->sel_count > MAX_MARK_QUADS ? MAX_MARK_QUADS : app->sel_count;
		memcpy(mark->quads, app->sel_quads,
			(size_t)mark->quad_count * sizeof(fz_quad));
		app->mark_count++;
	}
	app_save_marks(app);
	app_set_status(app, 0, L"Trecho marcado (%d marcações)", app->mark_count);
}

static void adopt_pending(App *app, int recent_index)
{
	Doc *doc;
	Recent *recent;
	int slot;

	capture_active(app);
	app->doc = app->pending_doc;
	memset(&app->pending_doc, 0, sizeof(Doc));
	app->pending_active = 0;
	doc = &app->doc;

	slot = app->tab_count;
	memset(&app->tabs[slot], 0, sizeof(Tab));
	app->tabs[slot].id = app->next_tab_id++;
	app->tab_count++;
	app->active_tab = slot;
	live_reset(app);

	doc_set_viewport(doc, app->view_w, app->view_h, app->dpi, app->ui);
	recent = recent_index >= 0 ? &app->state.recents[recent_index] : NULL;
	if (recent) {
		doc->fit = recent->fit;
		doc->zoom = clamp_zoom(recent->zoom > 0.0f ? recent->zoom : 1.0f);
		doc->rotation = ((recent->rotation % 360) + 360) % 360;
		doc_set_page(doc, recent->page);
	}
	marks_load(app);
	app->render_pending = 1;
	app->tab_changed = 1;
	app->password_pending = 0;
	app->password_popup = 0;
	app->password[0] = 0;
	app->password_error[0] = 0;
	app_set_status(app, 0, L"%s", base_name(doc->path));
	app_remember(app);
}

void app_load_document(App *app, const wchar_t *path, const char *password)
{
	wchar_t err[512] = L"";
	int code;
	int recent_index;
	int existing;

	if (!path || !path[0])
		return;
	existing = tab_find_path(app, path);
	if (existing >= 0) {
		app_switch_tab(app, existing);
		app->password_pending = 0;
		app->password_popup = 0;
		app->password[0] = 0;
		app->password_error[0] = 0;
		app_set_status(app, 0, L"%s", L"Este documento já está aberto.");
		return;
	}
	if (app->tab_count >= MAX_TABS) {
		app_set_status(app, 1, L"Limite de %d abas. Feche um documento.", MAX_TABS);
		app->password_pending = 0;
		return;
	}

	recent_index = state_find(&app->state, path);
	pending_release(app);
	doc_init(&app->pending_doc);
	app->pending_active = 1;
	code = doc_open(&app->pending_doc, path, password, err, 512);
	if (code == DOC_OK) {
		adopt_pending(app, recent_index);
		return;
	}
	pending_release(app);

	if (code == DOC_NEED_PASSWORD || code == DOC_BAD_PASSWORD) {
		wcsncpy(app->password_path, path, 1023);
		app->password_path[1023] = 0;
		app->password_pending = 1;
		app->password_popup = 0;
		app->password_focus = 1;
		app->password[0] = 0;
		wcsncpy(app->password_error,
			code == DOC_BAD_PASSWORD ? L"Senha incorreta. Tente novamente." : L"", 159);
		app->password_error[159] = 0;
		return;
	}

	app_set_status(app, 1, L"%s", err[0] ? err : L"Não foi possível abrir o documento.");
}

void app_open_path(App *app, const wchar_t *path)
{
	if (!path || !path[0])
		return;
	if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) {
		app_set_status(app, 1, L"Arquivo não encontrado: %s", base_name(path));
		return;
	}
	app_load_document(app, path, NULL);
}

void app_open_dialog(App *app)
{
	OPENFILENAMEW ofn;
	wchar_t file[1024] = L"";
	static const wchar_t filter[] =
		L"Documentos PDF (*.pdf)\0*.pdf\0Todos os arquivos\0*.*\0\0";

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = app->hwnd;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = file;
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = L"Abrir documento PDF";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
	if (GetOpenFileNameW(&ofn))
		app_open_path(app, file);
}

void app_submit_password(App *app)
{
	char password[128];
	wchar_t path[1024];
	if (!app->password_pending)
		return;
	memcpy(password, app->password, sizeof(password));
	password[127] = 0;
	wcsncpy(path, app->password_path, 1023);
	path[1023] = 0;
	app->password_pending = 0;
	app->password_popup = 0;
	app_load_document(app, path, password[0] ? password : NULL);
}

void app_cancel_password(App *app)
{
	pending_release(app);
	app->password_pending = 0;
	app->password_popup = 0;
	app->password[0] = 0;
	app->password_error[0] = 0;
	app->password_focus = 0;
}

void app_switch_tab(App *app, int index)
{
	if (index < 0 || index >= app->tab_count || index == app->active_tab)
		return;
	capture_active(app);
	app->active_tab = index;
	tab_load_live(app, &app->tabs[index]);
	ai_clear_result(app);
	app->ai_open = 0;
	app_set_status(app, 0, L"%s", base_name(app->doc.path));
}

void app_close_tab(App *app, int index)
{
	Tab *tab;
	int was_active;

	if (index < 0 || index >= app->tab_count)
		return;
	was_active = index == app->active_tab;
	tab = &app->tabs[index];
	if (was_active) {
		app_save_marks(app);
		doc_shutdown(&app->doc);
		memset(&app->doc, 0, sizeof(Doc));
		free(app->ocr_text);
		app->ocr_text = NULL;
		free(app->ocr_utf8);
		app->ocr_utf8 = NULL;
		live_reset(app);
		ai_clear_result(app);
		app->ai_open = 0;
		tab_remove_entry(app, index);
		app->active_tab = -1;
		if (app->tab_count > 0) {
			int next = index < app->tab_count ? index : app->tab_count - 1;
			app->active_tab = next;
			tab_load_live(app, &app->tabs[next]);
			app_set_status(app, 0, L"%s", base_name(app->doc.path));
		} else {
			app_set_status(app, 0, L"Documento fechado");
		}
	} else {
		doc_shutdown(&tab->doc);
		memset(&tab->doc, 0, sizeof(Doc));
		free(tab->ocr_text);
		tab->ocr_text = NULL;
		tab_remove_entry(app, index);
		if (index < app->active_tab)
			app->active_tab--;
	}
	app->tab_changed = 1;
	app_save_state(app);
}

void app_next_tab(App *app, int delta)
{
	int index;
	if (app->tab_count < 2)
		return;
	if (app->active_tab < 0)
		index = 0;
	else {
		index = (app->active_tab + delta) % app->tab_count;
		if (index < 0)
			index += app->tab_count;
	}
	app_switch_tab(app, index);
}

void app_mark_render(App *app)
{
	app->render_pending = 1;
}

void app_goto_page(App *app, int page)
{
	Doc *doc = &app->doc;
	int old = doc->page;
	if (!doc->doc)
		return;
	if (page < 0)
		page = 0;
	if (page >= doc->page_count)
		page = doc->page_count - 1;
	if (page == old && doc->page_obj && doc->pix)
		return;
	doc_set_page(doc, page);
	app->scroll_y = 0;
	app->scroll_x = 0;
	app->has_selection = 0;
	app->sel_count = 0;
	app->selecting = 0;
	app->render_pending = 1;
	app->sb_follow = 1;
	app_remember(app);
}

void app_step_page(App *app, int delta)
{
	if (!app->doc.doc)
		return;
	app_goto_page(app, app->doc.page + delta);
}

void app_set_zoom(App *app, float zoom)
{
	Doc *doc = &app->doc;
	if (!doc->doc)
		return;
	doc->fit = FIT_NONE;
	doc->zoom = clamp_zoom(zoom);
	app->scroll_x = 0;
	app->scroll_y = 0;
	app->render_pending = 1;
	app_remember(app);
}

void app_zoom_by(App *app, float factor)
{
	if (!app->doc.doc)
		return;
	app_set_zoom(app, app->doc.zoom * factor);
}

void app_fit(App *app, int mode)
{
	Doc *doc = &app->doc;
	if (!doc->doc)
		return;
	doc->fit = mode;
	app->scroll_x = 0;
	app->scroll_y = 0;
	app->render_pending = 1;
	app_remember(app);
}

void app_rotate(App *app)
{
	Doc *doc = &app->doc;
	if (!doc->doc)
		return;
	doc_rotate(doc, 90);
	app->scroll_x = 0;
	app->scroll_y = 0;
	app->has_selection = 0;
	app->sel_count = 0;
	app->render_pending = 1;
	app_thumbs_reset(app);
	app->sb_follow = 1;
	app_remember(app);
}

void app_render_tick(App *app)
{
	Doc *doc = &app->doc;
	unsigned char *samples;
	int stride, w, h;

	if (!doc->doc)
		return;
	doc_set_viewport(doc, app->view_w, app->view_h, app->dpi, app->ui);
	if (app->last_view_w != app->view_w || app->last_view_h != app->view_h) {
		app->last_view_w = app->view_w;
		app->last_view_h = app->view_h;
		if (doc->fit)
			app->render_pending = 1;
	}
	if (!app->render_pending && doc->pix && doc->pix_page == doc->page)
		return;
	if (!doc_render(doc)) {
		app_set_status(app, 1, L"%s", doc->error[0] ? doc->error : L"Falha ao renderizar a página.");
		app->render_pending = 0;
		return;
	}
	doc_ensure_stext(doc);
	w = fz_pixmap_width(doc->ctx, doc->pix);
	h = fz_pixmap_height(doc->ctx, doc->pix);
	stride = fz_pixmap_stride(doc->ctx, doc->pix);
	samples = fz_pixmap_samples(doc->ctx, doc->pix);
	if (gfx_page_texture(samples, w, h, stride)) {
		app->tex_id = gfx_page_texture_id();
		app->tex_page = doc->page;
		app->tex_rot = doc->rotation;
		app->tex_zoom = doc->zoom;
		app->tex_w = doc->page_w;
		app->tex_h = doc->page_h;
	}
	app->render_pending = 0;
}

char *app_selection_utf8_dup(App *app)
{
	if (!app->doc.doc || !app->has_selection || app->sel_count < 1)
		return NULL;
	return doc_selection_text(&app->doc, app->sel_a, app->sel_b);
}

void app_copy(App *app)
{
	char *text = app_selection_utf8_dup(app);
	int from_selection = text != NULL;

	if (!text)
		text = doc_page_text(&app->doc);
	if (!text || !text[0]) {
		free(text);
		app_set_status(app, 1, L"Nenhum texto selecionável nesta página. Use o OCR.");
		return;
	}
	{
		wchar_t *wide = mu_utf8_to_wide_alloc(text);
		if (wide && wide[0]) {
			copy_to_clipboard(app, wide);
			app_set_status(app, 0, from_selection ? L"Seleção copiada" : L"Texto da página copiado");
		} else {
			app_set_status(app, 1, L"Não foi possível copiar o texto.");
		}
		free(wide);
	}
	free(text);
}

static void export_dir_for(const App *app, wchar_t *out, size_t cap)
{
	const wchar_t *slash;
	size_t len;

	out[0] = 0;
	if (!app->doc.path)
		return;
	slash = wcsrchr(app->doc.path, L'\\');
	if (!slash)
		return;
	len = (size_t)(slash - app->doc.path);
	if (len >= cap)
		len = cap - 1;
	memcpy(out, app->doc.path, len * sizeof(wchar_t));
	out[len] = 0;
}

static void export_name_for(const App *app, wchar_t *out, size_t cap)
{
	const wchar_t *name = app->doc.path ? base_name(app->doc.path) : L"PDF Leve";
	wchar_t *dot;
	size_t len;

	wcsncpy(out, name, cap - 1);
	out[cap - 1] = 0;
	dot = wcsrchr(out, L'.');
	if (dot && dot != out)
		*dot = 0;
	len = wcslen(out);
	if (len == 0) {
		wcsncpy(out, L"PDF Leve", cap - 1);
		out[cap - 1] = 0;
	}
}

static void export_text_dialog(App *app, char *utf8, const wchar_t *suffix)
{
	OPENFILENAMEW ofn;
	wchar_t path[1024];
	wchar_t stem[256];
	wchar_t dir[1024];
	static const wchar_t filter[] =
		L"Textos (*.txt)\0*.txt\0Todos os arquivos\0*.*\0\0";
	const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
	FILE *file;
	size_t len;
	int ok;

	if (!utf8 || !utf8[0]) {
		app_set_status(app, 1, L"Não há texto para exportar.");
		return;
	}
	export_dir_for(app, dir, 1024);
	export_name_for(app, stem, 256);
	if (dir[0])
		_snwprintf(path, 1023, L"%s\\%s%s.txt", dir, stem, suffix);
	else
		_snwprintf(path, 1023, L"%s%s.txt", stem, suffix);
	path[1023] = 0;

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = app->hwnd;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = path;
	ofn.nMaxFile = 1024;
	ofn.lpstrDefExt = L"txt";
	ofn.lpstrTitle = L"Exportar texto";
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR |
		OFN_EXPLORER;
	if (!GetSaveFileNameW(&ofn))
		return;

	file = _wfopen(path, L"wb");
	if (!file) {
		app_set_status(app, 1, L"Não foi possível salvar o arquivo .txt.");
		return;
	}
	len = strlen(utf8);
	fwrite(bom, 1, sizeof(bom), file);
	fwrite(utf8, 1, len, file);
	if (len == 0 || utf8[len - 1] != '\n')
		fwrite("\r\n", 1, 2, file);
	ok = fclose(file) == 0;
	if (ok)
		app_set_status(app, 0, L"Texto exportado: %s", base_name(path));
	else
		app_set_status(app, 1, L"Não foi possível salvar o arquivo .txt.");
}

void app_export_page_text(App *app)
{
	wchar_t suffix[64];
	char *text;

	if (!app->doc.doc)
		return;
	text = doc_page_text(&app->doc);
	if (!text || !text[0]) {
		free(text);
		app_set_status(app, 1, L"Esta página não tem texto selecionável. Use o OCR e exporte o texto reconhecido.");
		return;
	}
	_snwprintf(suffix, 63, L" - página %d", app->doc.page + 1);
	suffix[63] = 0;
	export_text_dialog(app, text, suffix);
	free(text);
}

void app_export_ocr_text(App *app)
{
	wchar_t suffix[64];
	char *text;

	if (!app->ocr_text || !app->ocr_text[0]) {
		app_set_status(app, 1, L"Nenhum texto de OCR disponível. Reconheça uma página primeiro.");
		return;
	}
	text = mu_wide_to_utf8_alloc(app->ocr_text);
	if (!text || !text[0]) {
		free(text);
		app_set_status(app, 1, L"Não há texto para exportar.");
		return;
	}
	_snwprintf(suffix, 63, L" - OCR página %d",
		(app->ocr_page >= 0 ? app->ocr_page : 0) + 1);
	suffix[63] = 0;
	export_text_dialog(app, text, suffix);
	free(text);
}

void app_start_ocr(App *app)
{
	Doc *doc = &app->doc;
	wchar_t image[1024];
	wchar_t output[1024];

	if (!doc->doc || app->ocr_busy)
		return;

	if (ocr_cache_path(doc->path, doc->page, app->ocr_cache, 1024)) {
		wchar_t *cached = ocr_cache_read(app->ocr_cache);
		if (cached && cached[0]) {
			free(app->ocr_text);
			app->ocr_text = cached;
			app->ocr_page = doc->page;
			app->ocr_popup = 1;
			app->ocr_popup_shown = 0;
			app->ocr_focus_copied = 0;
			app_set_status(app, 0, L"Texto do OCR carregado do cache.");
			return;
		}
		free(cached);
	}

	ocr_temp_path(L".png", image, 1024);
	if (!doc_render_png(doc, 300, image)) {
		app_set_status(app, 1, L"Não foi possível preparar a página para o OCR.");
		return;
	}
	ocr_temp_path(L".txt", output, 1024);
	wcsncpy(app->ocr_source, image, 1023);
	app->ocr_source[1023] = 0;
	if (!ocr_launch(app->hwnd, image, output, app->ocr_cache, doc->page)) {
		DeleteFileW(image);
		app_set_status(app, 1, L"Não foi possível iniciar o OCR.");
		return;
	}
	app->ocr_owner_id = app->active_tab >= 0 ? app->tabs[app->active_tab].id : 0;
	InterlockedExchange(&app->ocr_busy, 1);
	app_set_status(app, 0, L"Reconhecendo texto da página %d…", doc->page + 1);
}

void app_ocr_done(App *app, OcrResult *result)
{
	int active_owner;
	Tab *owner;

	InterlockedExchange(&app->ocr_busy, 0);
	if (!result)
		return;
	active_owner = app->active_tab >= 0 &&
		app->tabs[app->active_tab].id == app->ocr_owner_id;
	owner = active_owner ? NULL : tab_by_id(app, app->ocr_owner_id);
	if (result->ok && result->text && result->text[0]) {
		if (active_owner) {
			free(app->ocr_text);
			app->ocr_text = result->text;
			result->text = NULL;
			app->ocr_page = result->page;
			app->ocr_popup = 1;
			app->ocr_popup_shown = 0;
			app->ocr_focus_copied = 0;
		} else if (owner) {
			free(owner->ocr_text);
			owner->ocr_text = result->text;
			result->text = NULL;
			owner->ocr_page = result->page;
			owner->ocr_popup = 1;
			owner->ocr_popup_shown = 0;
			owner->ocr_focus_copied = 0;
		}
		app_set_status(app, 0, L"Texto reconhecido na página %d.", result->page + 1);
	} else {
		app_set_status(app, 1, L"%s", result->error[0] ? result->error : L"Falha no OCR.");
	}
	free(result->text);
	free(result);
}

#define AI_CONTEXT_LIMIT 28000

static void ai_set_error(App *app, const char *text)
{
	free(app->ai_error);
	app->ai_error = text ? _strdup(text) : NULL;
}

static char *ai_context_dup(App *app)
{
	Doc *doc = &app->doc;
	char *text = NULL;
	size_t len;

	if (!doc->doc)
		return NULL;
	if (app->has_selection && app->sel_count > 0)
		text = app_selection_utf8_dup(app);
	if (!text || !text[0]) {
		free(text);
		text = doc_page_text(doc);
	}
	if ((!text || !text[0]) && app->ocr_text && app->ocr_page == doc->page) {
		free(text);
		text = mu_wide_to_utf8_alloc(app->ocr_text);
	}
	if (!text || !text[0]) {
		free(text);
		return NULL;
	}
	len = strlen(text);
	if (len > AI_CONTEXT_LIMIT) {
		static const char note[] = "\n\n[texto truncado pelo PDF Leve]";
		size_t cut = AI_CONTEXT_LIMIT;
		char *trimmed;
		while (cut > 0 && ((unsigned char)text[cut] & 0xC0) == 0x80)
			cut--;
		trimmed = (char *)malloc(cut + sizeof(note));
		if (!trimmed) {
			free(text);
			return NULL;
		}
		memcpy(trimmed, text, cut);
		memcpy(trimmed + cut, note, sizeof(note));
		free(text);
		text = trimmed;
	}
	return text;
}

void app_ai_context_label(const App *app, char *out, size_t cap)
{
	const Doc *doc = &app->doc;
	if (!doc->doc) {
		snprintf(out, cap, "%s", "Nenhum documento aberto");
		return;
	}
	if (app->has_selection && app->sel_count > 0)
		snprintf(out, cap, "Seleção · página %d", doc->page + 1);
	else if (app->ocr_text && app->ocr_page == doc->page && !doc->has_text)
		snprintf(out, cap, "Página %d · texto do OCR", doc->page + 1);
	else
		snprintf(out, cap, "Página %d inteira", doc->page + 1);
}

void app_ai_open_modal(App *app)
{
	if (app->ai_answer && app->ai_owner_id !=
		(app->active_tab >= 0 ? app->tabs[app->active_tab].id : 0))
		ai_clear_result(app);
	app->ai_open = 1;
	app->ai_setup = !app->ai_key_set;
	app->ai_key_input[0] = 0;
	app->ai_focus_key = app->ai_setup;
	app->ai_focus_question = !app->ai_setup;
	ai_set_error(app, NULL);
}

void app_ai_save_key(App *app)
{
	char *key = app->ai_key_input;
	size_t len;

	while (*key == ' ' || *key == '\t' || *key == '\r' || *key == '\n')
		key++;
	len = strlen(key);
	while (len > 0 && (key[len - 1] == ' ' || key[len - 1] == '\t' ||
		key[len - 1] == '\r' || key[len - 1] == '\n'))
		key[--len] = 0;
	if (!len) {
		ai_set_error(app, "Cole a chave da API do Google AI Studio.");
		return;
	}
	if (!gemini_save_key(key)) {
		ai_set_error(app, "Não foi possível salvar a chave neste computador.");
		return;
	}
	snprintf(app->ai_key, sizeof(app->ai_key), "%s", key);
	SecureZeroMemory(app->ai_key_input, sizeof(app->ai_key_input));
	app->ai_key_set = 1;
	app->ai_setup = 0;
	app->ai_focus_key = 0;
	app->ai_focus_question = 1;
	ai_set_error(app, NULL);
	app_set_status(app, 0, L"Chave do Gemini salva neste computador.");
}

void app_ai_remove_key(App *app)
{
	gemini_save_key(NULL);
	SecureZeroMemory(app->ai_key, sizeof(app->ai_key));
	SecureZeroMemory(app->ai_key_input, sizeof(app->ai_key_input));
	app->ai_key_set = 0;
	app->ai_setup = 1;
	app->ai_focus_key = 1;
	app->ai_focus_question = 0;
	ai_set_error(app, NULL);
	app_set_status(app, 0, L"Chave do Gemini removida deste computador.");
}

void app_ai_send(App *app, int summarize)
{
	static const char intro[] =
		"Você é um assistente de leitura dentro do aplicativo PDF Leve. "
		"Responda em português do Brasil, de forma clara e objetiva. "
		"Use somente o texto do documento fornecido; se a informação não estiver nele, "
		"diga que não a encontrou. Responda em texto simples, sem Markdown.\n\n";
	static const char summary_task[] =
		"Tarefa: resuma o texto abaixo em tópicos curtos e objetivos. "
		"Destaque datas, valores, nomes e números importantes quando existirem.\n\n";
	char question_task[768];
	char context_label[96];
	char *text;
	char *prompt;
	size_t need;
	int owner;

	if (!app->doc.doc) {
		ai_set_error(app, "Abra um documento para usar o Gemini.");
		return;
	}
	if (!app->ai_key_set) {
		app->ai_setup = 1;
		app->ai_focus_key = 1;
		return;
	}
	if (app->ai_busy)
		return;
	free(app->ai_answer);
	app->ai_answer = NULL;
	ai_set_error(app, NULL);
	if (!summarize) {
		const char *ask = app->ai_question;
		while (*ask == ' ' || *ask == '\t' || *ask == '\r' || *ask == '\n')
			ask++;
		if (!*ask) {
			ai_set_error(app, "Digite uma pergunta sobre o documento.");
			return;
		}
	}
	text = ai_context_dup(app);
	if (!text) {
		ai_set_error(app,
			"Não há texto nesta página. Selecione um trecho ou use o OCR.");
		return;
	}
	app_ai_context_label(app, context_label, sizeof(context_label));
	if (summarize)
		snprintf(question_task, sizeof(question_task), "%s", summary_task);
	else
		snprintf(question_task, sizeof(question_task),
			"Pergunta do usuário: %s\n\n", app->ai_question);
	need = strlen(intro) + strlen(question_task) + strlen(text) + 128;
	prompt = (char *)malloc(need);
	if (!prompt) {
		free(text);
		ai_set_error(app, "Memória insuficiente para montar a consulta.");
		return;
	}
	snprintf(prompt, need,
		"%s%s--- TEXTO DO DOCUMENTO ---\n%s\n--- FIM DO TEXTO ---",
		intro, question_task, text);
	free(text);
	owner = app->active_tab >= 0 ? app->tabs[app->active_tab].id : 0;
	if (!gemini_request_async(app->hwnd, app->ai_key, prompt, owner)) {
		free(prompt);
		ai_set_error(app, "Não foi possível iniciar a consulta ao Gemini.");
		app_set_status(app, 1, L"Falha ao iniciar a consulta ao Gemini.");
		return;
	}
	free(prompt);
	snprintf(app->ai_context, sizeof(app->ai_context), "%s", context_label);
	app->ai_owner_id = owner;
	app->ai_setup = 0;
	app->ai_open = 1;
	InterlockedExchange(&app->ai_busy, 1);
	app_set_status(app, 0, L"Consultando o Gemini…");
}

void app_ai_done(App *app, GeminiResult *result)
{
	int active_owner;

	InterlockedExchange(&app->ai_busy, 0);
	if (!result)
		return;
	active_owner = app->active_tab >= 0 ? app->tabs[app->active_tab].id : 0;
	if (result->owner_id != active_owner || !app->doc.doc) {
		app_set_status(app, 1,
			L"A resposta do Gemini foi descartada: o documento mudou.");
		free(result->text);
		free(result->error);
		free(result);
		return;
	}
	free(app->ai_answer);
	app->ai_answer = result->text;
	result->text = NULL;
	free(app->ai_error);
	app->ai_error = result->error;
	result->error = NULL;
	app->ai_owner_id = active_owner;
	app->ai_open = 1;
	app->ai_focus_question = 0;
	if (app->ai_answer && app->ai_answer[0]) {
		app_set_status(app, 0, L"Resposta do Gemini recebida.");
	} else {
		if (!app->ai_error)
			ai_set_error(app, "O Gemini não retornou uma resposta.");
		app_set_status(app, 1, L"O Gemini não retornou uma resposta.");
	}
	free(result);
}

void app_find_restart(App *app)
{
	memcpy(app->query_applied, app->query, sizeof(app->query));
	app->hit_count = 0;
	app->hit_index = -1;
	app->search_cursor = 0;
	app->searching = app->doc.doc && app->query[0] != 0;
}

void app_search_tick(App *app)
{
	Doc *doc = &app->doc;
	unsigned long long start;
	if (!app->searching || !doc->doc)
		return;
	start = GetTickCount64();
	while (app->search_cursor < doc->page_count) {
		fz_quad quads[64];
		int found;
		int i;
		int page = app->search_cursor;
		int room = MAX_HITS - app->hit_count;
		if (room <= 0) {
			app->searching = 0;
			break;
		}
		if (room > 64)
			room = 64;
		found = doc_search_page(doc, page, app->query, quads, room);
		for (i = 0; i < found; i++) {
			app->hits[app->hit_count].page = page;
			app->hits[app->hit_count].quad = quads[i];
			app->hit_count++;
		}
		app->search_cursor++;
		if (GetTickCount64() - start > 14)
			break;
	}
	if (app->search_cursor >= doc->page_count)
		app->searching = 0;
}

void app_find_close(App *app)
{
	app->find_open = 0;
	app->searching = 0;
	app->hit_count = 0;
	app->hit_index = -1;
}

void app_find_step(App *app, int delta)
{
	Doc *doc = &app->doc;
	int index;
	Hit *hit;
	fz_point center;
	int margin;

	if (!doc->doc)
		return;
	if (app->hit_count < 1) {
		if (app->query[0] && !app->searching)
			app_find_restart(app);
		return;
	}
	if (app->hit_index < 0) {
		index = delta >= 0 ? 0 : app->hit_count - 1;
	} else {
		index = (app->hit_index + delta) % app->hit_count;
		if (index < 0)
			index += app->hit_count;
	}
	app->hit_index = index;
	hit = &app->hits[index];
	if (hit->page != doc->page) {
		doc_set_page(doc, hit->page);
		app->render_pending = 1;
		app_remember(app);
	}
	if (!doc_render(doc)) {
		app->render_pending = 0;
		return;
	}
	if (doc->pix) {
		int w = fz_pixmap_width(doc->ctx, doc->pix);
		int h = fz_pixmap_height(doc->ctx, doc->pix);
		int stride = fz_pixmap_stride(doc->ctx, doc->pix);
		unsigned char *samples = fz_pixmap_samples(doc->ctx, doc->pix);
		if (gfx_page_texture(samples, w, h, stride)) {
			app->tex_id = gfx_page_texture_id();
			app->tex_page = doc->page;
			app->tex_rot = doc->rotation;
			app->tex_zoom = doc->zoom;
			app->tex_w = doc->page_w;
			app->tex_h = doc->page_h;
		}
	}
	app->render_pending = 0;

	center.x = (hit->quad.ul.x + hit->quad.ur.x + hit->quad.ll.x + hit->quad.lr.x) * 0.25f;
	center.y = (hit->quad.ul.y + hit->quad.ur.y + hit->quad.ll.y + hit->quad.lr.y) * 0.25f;
	if (doc->rotation)
		center = fz_transform_point(center, fz_rotate((float)doc->rotation));
	margin = MulDiv(VIEW_MARGIN, app->dpi, 96);
	{
		float cx = (center.x - doc->bounds.x0) * doc->scale;
		float cy = (center.y - doc->bounds.y0) * doc->scale;
		int max_x = (doc->page_w - app->view_w) / 2;
		if (max_x < 0)
			max_x = 0;
		app->scroll_max_x = max_x;
		app->scroll_x = (int)lround(cx - (float)doc->page_w * 0.5f);
		if (app->scroll_x < 0)
			app->scroll_x = 0;
		if (app->scroll_x > max_x)
			app->scroll_x = max_x;
		if (doc->page_h + margin * 2 > app->view_h) {
			int target = margin + (int)lround(cy) - app->view_h / 2;
			app->scroll_max = doc->page_h + margin * 2 - app->view_h;
			app->scroll_y = target;
			if (app->scroll_y < 0)
				app->scroll_y = 0;
			if (app->scroll_y > app->scroll_max)
				app->scroll_y = app->scroll_max;
		} else {
			app->scroll_y = 0;
			app->scroll_max = 0;
		}
	}
	app_set_status(app, 0, L"Ocorrência %d de %d · página %d", index + 1, app->hit_count,
		hit->page + 1);
}

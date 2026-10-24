#include "app.h"
#include "gfx.h"
#include "mu.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
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
	app->ui = (float)app->dpi / 96.0f;
	GetClientRect(hwnd, &rc);
	app->width = rc.right - rc.left;
	app->height = rc.bottom - rc.top;
	app->view_w = app->width;
	app->view_h = app->height;
	doc_init(&app->doc);
	doc_set_viewport(&app->doc, app->view_w, app->view_h, app->dpi);
	state_load(&app->state);
	app->hit_index = -1;
	app->tex_page = -1;
	app->ocr_page = -1;
}

void app_shutdown(App *app)
{
	app_save_state(app);
	free(app->ocr_text);
	app->ocr_text = NULL;
	free(app->ocr_utf8);
	app->ocr_utf8 = NULL;
	doc_shutdown(&app->doc);
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
	if (app->saving_disabled)
		return;
	app_remember(app);
	state_save(&app->state);
}

void app_load_document(App *app, const wchar_t *path, const char *password)
{
	Doc *doc = &app->doc;
	wchar_t err[512] = L"";
	int code;
	int index = state_find(&app->state, path);

	code = doc_open(doc, path, password, err, 512);
	if (code == DOC_OK) {
		Recent *recent = index >= 0 ? &app->state.recents[index] : NULL;
		doc_set_viewport(doc, app->view_w, app->view_h, app->dpi);
		if (recent) {
			doc->fit = recent->fit;
			doc->zoom = clamp_zoom(recent->zoom > 0.0f ? recent->zoom : 1.0f);
			doc->rotation = ((recent->rotation % 360) + 360) % 360;
			doc_set_page(doc, recent->page);
		}
		app->password_pending = 0;
		app->password_popup = 0;
		app->password[0] = 0;
		app->password_error[0] = 0;
		app->has_selection = 0;
		app->sel_count = 0;
		app->selecting = 0;
		app->scroll_x = 0;
		app->scroll_y = 0;
		app->hit_count = 0;
		app->hit_index = -1;
		app->searching = 0;
		app->query_applied[0] = 0;
		app->render_pending = 1;
		app->tex_id = 0;
		app->tex_page = -1;
		free(app->ocr_text);
		app->ocr_text = NULL;
		free(app->ocr_utf8);
		app->ocr_utf8 = NULL;
		app->ocr_popup = 0;
		app->ocr_popup_shown = 0;
		app->ocr_focus_copied = 0;
		app->ocr_page = -1;
		app_set_status(app, 0, L"%s", base_name(doc->path));
		app_remember(app);
		if (app->find_open && app->query[0])
			app_find_restart(app);
		return;
	}

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
	app->password_pending = 0;
	app->password_popup = 0;
	app->password[0] = 0;
	app->password_error[0] = 0;
	app->password_focus = 0;
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
	app_remember(app);
}

void app_render_tick(App *app)
{
	Doc *doc = &app->doc;
	unsigned char *samples;
	int stride, w, h;

	if (!doc->doc)
		return;
	doc_set_viewport(doc, app->view_w, app->view_h, app->dpi);
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
	InterlockedExchange(&app->ocr_busy, 1);
	app_set_status(app, 0, L"Reconhecendo texto da página %d…", doc->page + 1);
}

void app_ocr_done(App *app, OcrResult *result)
{
	InterlockedExchange(&app->ocr_busy, 0);
	if (!result)
		return;
	if (result->ok && result->text && result->text[0]) {
		free(app->ocr_text);
		app->ocr_text = result->text;
		result->text = NULL;
		app->ocr_page = result->page;
		app->ocr_popup = 1;
		app->ocr_popup_shown = 0;
		app->ocr_focus_copied = 0;
		app_set_status(app, 0, L"Texto reconhecido na página %d.", result->page + 1);
	} else {
		app_set_status(app, 1, L"%s", result->error[0] ? result->error : L"Falha no OCR.");
	}
	free(result->text);
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

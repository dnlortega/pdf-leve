#ifndef PDF_LEVE_DOC_H
#define PDF_LEVE_DOC_H

#include <stddef.h>
#include <wchar.h>

#include "mu.h"

#define VIEW_MARGIN 28
#define MIN_ZOOM 0.15f
#define MAX_ZOOM 6.0f
#define MAX_RENDER_PIXELS 12000000.0
#define MAX_RENDER_SIDE 5200.0

enum {
	DOC_OK = 0,
	DOC_NEED_PASSWORD,
	DOC_BAD_PASSWORD,
	DOC_ERROR
};

enum {
	FIT_NONE = 0,
	FIT_WIDTH = 1,
	FIT_PAGE = 2
};

typedef struct Doc {
	fz_context *ctx;
	fz_document *doc;
	fz_page *page_obj;
	fz_pixmap *pix;
	fz_stext_page *stext;

	wchar_t *path;
	char *password;

	int page_count;
	int page;
	int rotation;
	float zoom;
	int fit;
	int view_w;
	int view_h;
	int dpi;

	float scale;
	float shrink;
	int page_w;
	int page_h;
	fz_rect bounds;
	fz_rect page_bounds;
	int pix_page;

	int stext_page;
	int has_text;
	int encrypted;
	int copy_allowed;

	wchar_t error[512];
} Doc;

void doc_init(Doc *d);
void doc_shutdown(Doc *d);
void doc_close(Doc *d);

int doc_open(Doc *d, const wchar_t *path, const char *password, wchar_t *err, size_t errcap);
int doc_set_page(Doc *d, int page);
int doc_render(Doc *d);
int doc_ensure_stext(Doc *d);
char *doc_page_text(Doc *d);
char *doc_selection_text(Doc *d, fz_point a, fz_point b);
int doc_selection_quads(Doc *d, fz_point a, fz_point b, fz_quad *quads, int max_quads);
int doc_word_at(Doc *d, fz_point point, fz_point *a, fz_point *b, fz_quad *quad);
int doc_search_page(Doc *d, int page, const char *needle, fz_quad *quads, int max_quads);
int doc_render_png(Doc *d, int dpi, const wchar_t *path);
void doc_set_viewport(Doc *d, int width, int height, int dpi);
float doc_fit_zoom(Doc *d);
void doc_rotate(Doc *d, int degrees);
void doc_invalidate(Doc *d);

#endif

#include "doc.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_error(Doc *d, const wchar_t *text)
{
	wcsncpy(d->error, text, 511);
	d->error[511] = 0;
}

static void set_error_from_utf8(Doc *d, const char *text)
{
	wchar_t *wide = mu_utf8_to_wide_alloc(text ? text : "erro desconhecido");
	if (wide) {
		set_error(d, wide);
		free(wide);
	} else {
		set_error(d, L"Erro ao abrir o documento.");
	}
}

void doc_init(Doc *d)
{
	memset(d, 0, sizeof(*d));
	d->zoom = 1.0f;
	d->fit = FIT_WIDTH;
	d->ui = 1.0f;
	d->ctx = mu_new_context();
}

void doc_invalidate(Doc *d)
{
	if (d->pix) {
		fz_drop_pixmap(d->ctx, d->pix);
		d->pix = NULL;
	}
	if (d->stext) {
		fz_drop_stext_page(d->ctx, d->stext);
		d->stext = NULL;
	}
	d->stext_page = -1;
	d->has_text = 0;
	d->pix_page = -1;
}

void doc_close(Doc *d)
{
	if (!d->ctx)
		return;
	doc_invalidate(d);
	if (d->page_obj) {
		fz_drop_page(d->ctx, d->page_obj);
		d->page_obj = NULL;
	}
	if (d->doc) {
		fz_drop_document(d->ctx, d->doc);
		d->doc = NULL;
	}
	free(d->path);
	d->path = NULL;
	free(d->password);
	d->password = NULL;
	d->page_count = 0;
	d->page = 0;
	d->rotation = 0;
	d->encrypted = 0;
	d->copy_allowed = 1;
	d->page_w = 0;
	d->page_h = 0;
	d->scale = 1.0f;
	d->shrink = 1.0f;
	memset(&d->bounds, 0, sizeof(d->bounds));
}

void doc_shutdown(Doc *d)
{
	doc_close(d);
	if (d->ctx) {
		fz_drop_context(d->ctx);
		d->ctx = NULL;
	}
}

int doc_open(Doc *d, const wchar_t *path, const char *password, wchar_t *err, size_t errcap)
{
	char *upath;
	int code = DOC_OK;
	int encrypted = 0;
	wchar_t message[512] = L"";

	fz_var(code);
	fz_var(encrypted);

	if (!d->ctx)
		d->ctx = mu_new_context();
	if (!d->ctx) {
		wcsncpy(err, L"Motor de PDF não inicializado.", errcap - 1);
		err[errcap - 1] = 0;
		return DOC_ERROR;
	}

	doc_close(d);
	upath = mu_wide_to_utf8_alloc(path);
	if (!upath) {
		wcsncpy(err, L"Caminho de arquivo inválido.", errcap - 1);
		err[errcap - 1] = 0;
		return DOC_ERROR;
	}

	fz_try(d->ctx) {
		d->doc = fz_open_document(d->ctx, upath);
		if (!d->doc)
			fz_throw(d->ctx, FZ_ERROR_FORMAT, "não foi possível abrir o documento");
		if (fz_needs_password(d->ctx, d->doc)) {
			encrypted = 1;
			if (!password) {
				code = DOC_NEED_PASSWORD;
			} else if (!fz_authenticate_password(d->ctx, d->doc, password)) {
				code = DOC_BAD_PASSWORD;
			}
		}
		if (code == DOC_OK) {
			d->page_count = fz_count_pages(d->ctx, d->doc);
			if (d->page_count < 1)
				fz_throw(d->ctx, FZ_ERROR_FORMAT, "documento sem páginas");
			d->copy_allowed = fz_has_permission(d->ctx, d->doc, FZ_PERMISSION_COPY);
			d->path = _wcsdup(path);
			d->password = password ? _strdup(password) : NULL;
			d->encrypted = encrypted;
			d->page = 0;
			d->rotation = 0;
			d->zoom = 1.0f;
			d->fit = FIT_WIDTH;
			d->stext_page = -1;
			d->pix_page = -1;
			d->has_text = 0;
		}
	}
	fz_catch(d->ctx) {
		code = DOC_ERROR;
		set_error_from_utf8(d, fz_caught_message(d->ctx));
		wcsncpy(message, d->error, 511);
		message[511] = 0;
	}

	free(upath);

	if (code != DOC_OK) {
		bool was_encrypted = encrypted != 0;
		doc_close(d);
		d->encrypted = was_encrypted;
		if (code == DOC_NEED_PASSWORD)
			set_error(d, L"Este PDF exige uma senha de abertura.");
		else if (code == DOC_BAD_PASSWORD)
			set_error(d, L"Senha incorreta.");
		else if (message[0])
			set_error(d, message);
		wcsncpy(err, d->error, errcap - 1);
		err[errcap - 1] = 0;
		return code;
	}

	err[0] = 0;
	return DOC_OK;
}

void doc_set_viewport(Doc *d, int width, int height, int dpi, float ui)
{
	d->view_w = width > 0 ? width : 1;
	d->view_h = height > 0 ? height : 1;
	d->dpi = dpi > 0 ? dpi : 96;
	d->ui = ui > 0.0f ? ui : 1.0f;
}

static int ensure_page(Doc *d)
{
	if (!d->doc)
		return 0;
	if (d->page_obj)
		return 1;
	fz_try(d->ctx) {
		d->page_obj = fz_load_page(d->ctx, d->doc, d->page);
	}
	fz_catch(d->ctx) {
		set_error_from_utf8(d, fz_caught_message(d->ctx));
		return 0;
	}
	return d->page_obj != NULL;
}

static fz_rect rotated_bounds(Doc *d)
{
	fz_rect bounds = fz_bound_page(d->ctx, d->page_obj);
	if (d->rotation)
		bounds = fz_transform_rect(bounds, fz_rotate((float)d->rotation));
	return bounds;
}

float doc_fit_zoom(Doc *d)
{
	fz_rect bounds;
	float avail_w, avail_h, zoom;
	if (!ensure_page(d))
		return d->zoom;
	bounds = rotated_bounds(d);
	if (bounds.x1 <= bounds.x0 || bounds.y1 <= bounds.y0)
		return 1.0f;
	float margin = (float)VIEW_MARGIN * d->ui;
	avail_w = (float)d->view_w - margin * 2.0f;
	avail_h = (float)d->view_h - margin * 2.0f;
	if (avail_w < 80.0f)
		avail_w = 80.0f;
	if (avail_h < 80.0f)
		avail_h = 80.0f;
	if (d->fit == FIT_PAGE) {
		float zx = avail_w / (bounds.x1 - bounds.x0);
		float zy = avail_h / (bounds.y1 - bounds.y0);
		zoom = (zx < zy ? zx : zy) * 72.0f / (float)d->dpi;
	} else {
		zoom = (avail_w / (bounds.x1 - bounds.x0)) * 72.0f / (float)d->dpi;
	}
	if (zoom < MIN_ZOOM)
		zoom = MIN_ZOOM;
	if (zoom > MAX_ZOOM)
		zoom = MAX_ZOOM;
	return zoom;
}

int doc_render(Doc *d)
{
	fz_pixmap *pix = NULL;
	fz_device *dev = NULL;
	fz_rect bounds, transformed;
	fz_matrix rotate, scale, ctm, draw;
	fz_irect ibox;
	float target_scale, render_scale, shrink = 1.0f;
	float width_px, height_px;
	int w, h;

	if (!d->doc || d->page < 0 || d->page >= d->page_count)
		return 0;
	if (!ensure_page(d))
		return 0;

	if (d->fit)
		d->zoom = doc_fit_zoom(d);
	if (d->zoom < MIN_ZOOM)
		d->zoom = MIN_ZOOM;
	if (d->zoom > MAX_ZOOM)
		d->zoom = MAX_ZOOM;

	bounds = rotated_bounds(d);
	target_scale = d->zoom * (float)d->dpi / 72.0f;
	width_px = (bounds.x1 - bounds.x0) * target_scale;
	height_px = (bounds.y1 - bounds.y0) * target_scale;
	if (width_px < 1.0f)
		width_px = 1.0f;
	if (height_px < 1.0f)
		height_px = 1.0f;

	if (width_px * height_px > MAX_RENDER_PIXELS)
		shrink = (float)sqrt(MAX_RENDER_PIXELS / ((double)width_px * height_px));
	if (width_px * shrink > MAX_RENDER_SIDE)
		shrink = (float)(MAX_RENDER_SIDE / width_px);
	if (height_px * shrink > MAX_RENDER_SIDE)
		shrink = (float)(MAX_RENDER_SIDE / height_px);
	if (shrink > 1.0f)
		shrink = 1.0f;

	render_scale = target_scale * shrink;
	rotate = fz_rotate((float)d->rotation);
	scale = fz_scale(render_scale, render_scale);
	ctm = fz_concat(rotate, scale);
	transformed = fz_transform_rect(fz_bound_page(d->ctx, d->page_obj), ctm);
	ibox = fz_round_rect(transformed);
	w = ibox.x1 - ibox.x0;
	h = ibox.y1 - ibox.y0;
	if (w < 1 || h < 1) {
		set_error(d, L"Página com dimensões inválidas.");
		return 0;
	}
	draw = fz_concat(ctm, fz_translate((float)-ibox.x0, (float)-ibox.y0));

	fz_var(pix);
	fz_var(dev);

	fz_try(d->ctx) {
		pix = fz_new_pixmap(d->ctx, fz_device_bgr(d->ctx), w, h, NULL, 1);
		fz_clear_pixmap_with_value(d->ctx, pix, 255);
		/* The draw device already maps page space to the pixmap; the page
		   itself must be run without an extra transform. */
		dev = fz_new_draw_device(d->ctx, draw, pix);
		fz_run_page(d->ctx, d->page_obj, dev, fz_scale(1.0f, 1.0f), NULL);
		fz_close_device(d->ctx, dev);
	}
	fz_always(d->ctx) {
		fz_drop_device(d->ctx, dev);
	}
	fz_catch(d->ctx) {
		fz_drop_pixmap(d->ctx, pix);
		set_error_from_utf8(d, fz_caught_message(d->ctx));
		return 0;
	}

	if (d->pix)
		fz_drop_pixmap(d->ctx, d->pix);
	d->pix = pix;
	d->pix_page = d->page;
	d->scale = target_scale;
	d->shrink = shrink;
	d->bounds = bounds;
	d->page_bounds = fz_bound_page(d->ctx, d->page_obj);
	d->page_w = (int)floor((double)(bounds.x1 - bounds.x0) * target_scale + 0.5);
	d->page_h = (int)floor((double)(bounds.y1 - bounds.y0) * target_scale + 0.5);
	if (d->page_w < 1)
		d->page_w = 1;
	if (d->page_h < 1)
		d->page_h = 1;
	return 1;
}

int doc_set_page(Doc *d, int page)
{
	if (!d->doc)
		return 0;
	if (page < 0)
		page = 0;
	if (page >= d->page_count)
		page = d->page_count - 1;
	if (page == d->page && d->page_obj)
		return 0;
	if (d->page_obj) {
		fz_drop_page(d->ctx, d->page_obj);
		d->page_obj = NULL;
	}
	d->page = page;
	doc_invalidate(d);
	return 1;
}

void doc_rotate(Doc *d, int degrees)
{
	d->rotation = (d->rotation + degrees) % 360;
	if (d->rotation < 0)
		d->rotation += 360;
	doc_invalidate(d);
}

int doc_ensure_stext(Doc *d)
{
	fz_stext_block *block;
	if (!ensure_page(d))
		return 0;
	if (d->stext && d->stext_page == d->page)
		return 1;
	if (d->stext) {
		fz_drop_stext_page(d->ctx, d->stext);
		d->stext = NULL;
	}
	fz_try(d->ctx) {
		d->stext = fz_new_stext_page_from_page(d->ctx, d->page_obj, NULL);
	}
	fz_catch(d->ctx) {
		set_error_from_utf8(d, fz_caught_message(d->ctx));
		return 0;
	}
	d->stext_page = d->page;
	d->has_text = 0;
	for (block = d->stext ? d->stext->first_block : NULL; block; block = block->next) {
		if (block->type == FZ_STEXT_BLOCK_TEXT) {
			fz_stext_line *line;
			for (line = block->u.t.first_line; line; line = line->next) {
				if (line->first_char) {
					d->has_text = 1;
					break;
				}
			}
		}
		if (d->has_text)
			break;
	}
	return 1;
}

char *doc_page_text(Doc *d)
{
	fz_buffer *buf = NULL;
	fz_output *out = NULL;
	unsigned char *data;
	size_t len;
	char *copy = NULL;

	if (!doc_ensure_stext(d) || !d->has_text)
		return NULL;

	fz_var(buf);
	fz_var(out);
	fz_var(copy);

	fz_try(d->ctx) {
		buf = fz_new_buffer(d->ctx, 4096);
		out = fz_new_output_with_buffer(d->ctx, buf);
		fz_print_stext_page_as_text(d->ctx, out, d->stext);
		fz_close_output(d->ctx, out);
		out = NULL;
		len = fz_buffer_storage(d->ctx, buf, &data);
		if (len > 0) {
			copy = (char *)malloc(len + 1);
			if (copy) {
				memcpy(copy, data, len);
				copy[len] = 0;
			}
		}
	}
	fz_always(d->ctx) {
		fz_drop_output(d->ctx, out);
	}
	fz_catch(d->ctx) {
		free(copy);
		copy = NULL;
		set_error_from_utf8(d, fz_caught_message(d->ctx));
	}
	fz_drop_buffer(d->ctx, buf);
	return copy;
}

char *doc_selection_text(Doc *d, fz_point a, fz_point b)
{
	char *raw = NULL;
	char *copy = NULL;
	if (!doc_ensure_stext(d))
		return NULL;
	fz_var(raw);
	fz_var(copy);
	fz_try(d->ctx) {
		raw = fz_copy_selection(d->ctx, d->stext, a, b, 0);
		if (raw) {
			size_t len = strlen(raw);
			copy = (char *)malloc(len + 1);
			if (copy)
				memcpy(copy, raw, len + 1);
		}
	}
	fz_catch(d->ctx) {
		free(copy);
		copy = NULL;
	}
	if (raw)
		fz_free(d->ctx, raw);
	return copy;
}

int doc_selection_quads(Doc *d, fz_point a, fz_point b, fz_quad *quads, int max_quads)
{
	int count = 0;
	if (!doc_ensure_stext(d))
		return 0;
	fz_try(d->ctx) {
		count = fz_highlight_selection(d->ctx, d->stext, a, b, quads, max_quads);
	}
	fz_catch(d->ctx) {
		count = 0;
	}
	return count;
}

int doc_word_at(Doc *d, fz_point point, fz_point *a, fz_point *b, fz_quad *quad)
{
	fz_point pa = point;
	fz_point pb = point;
	fz_quad box;
	int ok = 0;
	if (!doc_ensure_stext(d))
		return 0;
	fz_try(d->ctx) {
		box = fz_snap_selection(d->ctx, d->stext, &pa, &pb, FZ_SELECT_WORDS);
		ok = 1;
	}
	fz_catch(d->ctx) {
		ok = 0;
	}
	if (ok) {
		if (a)
			*a = pa;
		if (b)
			*b = pb;
		if (quad)
			*quad = box;
	}
	return ok;
}

int doc_search_page(Doc *d, int page, const char *needle, fz_quad *quads, int max_quads)
{
	int count = 0;
	if (!d->doc || !needle || !needle[0])
		return 0;
	fz_var(count);
	if (!ensure_page(d) || page != d->page) {
		/* Search a page that is not loaded by loading it temporarily. */
		fz_page *temp = NULL;
		fz_var(temp);
		fz_try(d->ctx) {
			temp = fz_load_page(d->ctx, d->doc, page);
			count = fz_search_page(d->ctx, temp, needle, NULL, quads, max_quads);
		}
		fz_always(d->ctx) {
			fz_drop_page(d->ctx, temp);
		}
		fz_catch(d->ctx) {
			count = 0;
		}
		return count;
	}
	fz_try(d->ctx) {
		count = fz_search_page(d->ctx, d->page_obj, needle, NULL, quads, max_quads);
	}
	fz_catch(d->ctx) {
		count = 0;
	}
	return count;
}

int doc_render_png(Doc *d, int dpi, const wchar_t *path)
{
	fz_pixmap *pix = NULL;
	fz_device *dev = NULL;
	fz_rect bounds, transformed;
	fz_matrix ctm, draw;
	fz_irect ibox;
	char *upath = NULL;
	float scale, shrink = 1.0f;
	int w, h;
	int ok = 0;

	if (!ensure_page(d))
		return 0;
	upath = mu_wide_to_utf8_alloc(path);
	if (!upath)
		return 0;
	bounds = rotated_bounds(d);
	scale = (float)dpi / 72.0f;
	{
		double px_w = (bounds.x1 - bounds.x0) * scale;
		double px_h = (bounds.y1 - bounds.y0) * scale;
		if (px_w * px_h > 16000000.0)
			shrink = (float)sqrt(16000000.0 / (px_w * px_h));
		if (px_w * shrink > 4200.0)
			shrink = (float)(4200.0 / px_w);
		if (px_h * shrink > 4200.0)
			shrink = (float)(4200.0 / px_h);
	}
	scale *= shrink;
	ctm = fz_concat(fz_rotate((float)d->rotation), fz_scale(scale, scale));
	transformed = fz_transform_rect(fz_bound_page(d->ctx, d->page_obj), ctm);
	ibox = fz_round_rect(transformed);
	w = ibox.x1 - ibox.x0;
	h = ibox.y1 - ibox.y0;
	if (w < 1 || h < 1) {
		free(upath);
		return 0;
	}
	draw = fz_concat(ctm, fz_translate((float)-ibox.x0, (float)-ibox.y0));

	fz_var(pix);
	fz_var(dev);
	fz_var(ok);

	fz_try(d->ctx) {
		pix = fz_new_pixmap(d->ctx, fz_device_rgb(d->ctx), w, h, NULL, 0);
		fz_clear_pixmap_with_value(d->ctx, pix, 255);
		dev = fz_new_draw_device(d->ctx, draw, pix);
		fz_run_page(d->ctx, d->page_obj, dev, fz_scale(1.0f, 1.0f), NULL);
		fz_close_device(d->ctx, dev);
		fz_save_pixmap_as_png(d->ctx, pix, upath);
		ok = 1;
	}
	fz_always(d->ctx) {
		fz_drop_device(d->ctx, dev);
	}
	fz_catch(d->ctx) {
		ok = 0;
		set_error_from_utf8(d, fz_caught_message(d->ctx));
	}
	fz_drop_pixmap(d->ctx, pix);
	free(upath);
	return ok;
}

int doc_render_thumb(Doc *d, int page, int max_w, int max_h,
	unsigned char **pixels, int *width, int *height, int *stride)
{
	fz_page *page_obj = NULL;
	fz_page *temp = NULL;
	fz_pixmap *pix = NULL;
	fz_device *dev = NULL;
	fz_rect bounds, transformed;
	fz_matrix ctm, draw;
	fz_irect ibox;
	unsigned char *copy = NULL;
	unsigned char *samples;
	float scale_x, scale_y, scale;
	int w = 0, h = 0, pix_stride = 0;
	int row;
	int ok = 0;

	if (!d->doc || page < 0 || page >= d->page_count || max_w < 1 || max_h < 1 ||
		!pixels)
		return 0;
	*pixels = NULL;
	if (width)
		*width = 0;
	if (height)
		*height = 0;
	if (stride)
		*stride = 0;

	fz_var(temp);
	fz_var(page_obj);
	fz_var(pix);
	fz_var(dev);
	fz_var(copy);
	fz_var(w);
	fz_var(h);
	fz_var(pix_stride);
	fz_var(ok);

	if (page == d->page && d->page_obj) {
		page_obj = d->page_obj;
	} else {
		fz_try(d->ctx) {
			temp = fz_load_page(d->ctx, d->doc, page);
		}
		fz_catch(d->ctx) {
			temp = NULL;
		}
		page_obj = temp;
	}
	if (!page_obj)
		return 0;

	bounds = fz_bound_page(d->ctx, page_obj);
	if (d->rotation)
		bounds = fz_transform_rect(bounds, fz_rotate((float)d->rotation));
	if (bounds.x1 <= bounds.x0 || bounds.y1 <= bounds.y0)
		goto done;
	scale_x = (float)max_w / (bounds.x1 - bounds.x0);
	scale_y = (float)max_h / (bounds.y1 - bounds.y0);
	scale = scale_x < scale_y ? scale_x : scale_y;
	if (scale <= 0.0f)
		goto done;
	ctm = fz_concat(fz_rotate((float)d->rotation), fz_scale(scale, scale));
	transformed = fz_transform_rect(fz_bound_page(d->ctx, page_obj), ctm);
	ibox = fz_round_rect(transformed);
	w = ibox.x1 - ibox.x0;
	h = ibox.y1 - ibox.y0;
	if (w < 1 || h < 1)
		goto done;
	draw = fz_concat(ctm, fz_translate((float)-ibox.x0, (float)-ibox.y0));

	fz_try(d->ctx) {
		pix = fz_new_pixmap(d->ctx, fz_device_bgr(d->ctx), w, h, NULL, 1);
		fz_clear_pixmap_with_value(d->ctx, pix, 255);
		dev = fz_new_draw_device(d->ctx, draw, pix);
		fz_run_page(d->ctx, page_obj, dev, fz_scale(1.0f, 1.0f), NULL);
		fz_close_device(d->ctx, dev);
		fz_drop_device(d->ctx, dev);
		dev = NULL;
		pix_stride = fz_pixmap_stride(d->ctx, pix);
		samples = fz_pixmap_samples(d->ctx, pix);
		copy = (unsigned char *)malloc((size_t)w * (size_t)h * 4);
		if (!copy)
			fz_throw(d->ctx, FZ_ERROR_GENERIC, "memória insuficiente para a miniatura");
		for (row = 0; row < h; row++)
			memcpy(copy + (size_t)row * (size_t)w * 4,
				samples + (size_t)row * (size_t)pix_stride, (size_t)w * 4);
		ok = 1;
	}
	fz_always(d->ctx) {
		fz_drop_device(d->ctx, dev);
	}
	fz_catch(d->ctx) {
		free(copy);
		copy = NULL;
		ok = 0;
	}
	fz_drop_pixmap(d->ctx, pix);
	pix = NULL;

done:
	if (temp)
		fz_drop_page(d->ctx, temp);
	if (!ok)
		return 0;
	*pixels = copy;
	if (width)
		*width = w;
	if (height)
		*height = h;
	if (stride)
		*stride = w * 4;
	return 1;
}

static void outline_collect(Doc *d, fz_outline *item, int depth,
	DocOutlineItem *items, int max_items, int *count)
{
	fz_var(item);
	while (item && *count < max_items) {
		DocOutlineItem *out = &items[*count];
		wchar_t *wide;
		int page = -1;

		if (item->page.chapter == 0 && item->page.page >= 0) {
			page = item->page.page;
		} else if (item->uri && item->uri[0]) {
			float x = 0.0f;
			float y = 0.0f;
			fz_location loc;
			fz_var(loc);
			fz_try(d->ctx) {
				loc = fz_resolve_link(d->ctx, d->doc, item->uri, &x, &y);
			}
			fz_catch(d->ctx) {
				loc.chapter = -1;
				loc.page = -1;
			}
			if (loc.chapter == 0 && loc.page >= 0)
				page = loc.page;
		}
		if (page < 0 || page >= d->page_count)
			page = -1;

		memset(out, 0, sizeof(*out));
		out->page = page;
		out->depth = depth;
		wide = mu_utf8_to_wide_alloc(item->title ? item->title : "");
		if (wide) {
			wcsncpy(out->title, wide, 255);
			out->title[255] = 0;
			free(wide);
		}
		if (!out->title[0])
			wcscpy(out->title, L"Sem título");
		(*count)++;
		if (item->down)
			outline_collect(d, item->down, depth < 7 ? depth + 1 : depth,
				items, max_items, count);
		item = item->next;
	}
}

int doc_load_outline(Doc *d, DocOutlineItem *items, int max_items)
{
	fz_outline *outline = NULL;
	int count = 0;

	if (!d->doc || !items || max_items < 1)
		return 0;
	fz_var(outline);
	fz_try(d->ctx) {
		outline = fz_load_outline(d->ctx, d->doc);
	}
	fz_catch(d->ctx) {
		outline = NULL;
	}
	if (!outline)
		return 0;
	outline_collect(d, outline, 0, items, max_items, &count);
	fz_drop_outline(d->ctx, outline);
	return count;
}

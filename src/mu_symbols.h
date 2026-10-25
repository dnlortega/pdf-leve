#ifndef PDF_LEVE_MU_SYMBOLS_H
#define PDF_LEVE_MU_SYMBOLS_H

#include <windows.h>
#include <mupdf/fitz.h>

/* Every MuPDF entry point is resolved at runtime so the reader can carry the
   DLL inside its own executable instead of depending on a side-by-side file. */
#define MU_FUNCS(X) \
	X(fz_new_context_imp) \
	X(fz_register_document_handlers) \
	X(fz_drop_context) \
	X(fz_var_imp) \
	X(fz_push_try) \
	X(fz_do_try) \
	X(fz_do_always) \
	X(fz_do_catch) \
	X(fz_throw) \
	X(fz_caught_message) \
	X(fz_open_document) \
	X(fz_needs_password) \
	X(fz_authenticate_password) \
	X(fz_has_permission) \
	X(fz_count_pages) \
	X(fz_load_page) \
	X(fz_bound_page) \
	X(fz_drop_page) \
	X(fz_drop_document) \
	X(fz_page_label) \
	X(fz_load_outline) \
	X(fz_drop_outline) \
	X(fz_resolve_link) \
	X(fz_scale) \
	X(fz_rotate) \
	X(fz_concat) \
	X(fz_translate) \
	X(fz_transform_rect) \
	X(fz_transform_point) \
	X(fz_round_rect) \
	X(fz_new_pixmap) \
	X(fz_clear_pixmap_with_value) \
	X(fz_new_draw_device) \
	X(fz_run_page) \
	X(fz_close_device) \
	X(fz_drop_device) \
	X(fz_drop_pixmap) \
	X(fz_device_rgb) \
	X(fz_device_bgr) \
	X(fz_pixmap_width) \
	X(fz_pixmap_height) \
	X(fz_pixmap_stride) \
	X(fz_pixmap_samples) \
	X(fz_save_pixmap_as_png) \
	X(fz_new_stext_page_from_page) \
	X(fz_drop_stext_page) \
	X(fz_search_page) \
	X(fz_snap_selection) \
	X(fz_highlight_selection) \
	X(fz_copy_selection) \
	X(fz_free) \
	X(fz_new_buffer) \
	X(fz_new_output_with_buffer) \
	X(fz_print_stext_page_as_text) \
	X(fz_close_output) \
	X(fz_drop_output) \
	X(fz_buffer_storage) \
	X(fz_drop_buffer)

#define MU_DECL(name) extern __typeof__(name) *mu_p_##name;
MU_FUNCS(MU_DECL)
#undef MU_DECL

#endif

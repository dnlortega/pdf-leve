#ifndef PDF_LEVE_MU_H
#define PDF_LEVE_MU_H

#include "mu_symbols.h"

/* Route every MuPDF call made by the application through the resolved
   pointers. Headers must be included before these aliases. */
#define fz_new_context_imp mu_p_fz_new_context_imp
#define fz_register_document_handlers mu_p_fz_register_document_handlers
#define fz_drop_context mu_p_fz_drop_context
#define fz_var_imp mu_p_fz_var_imp
#define fz_push_try mu_p_fz_push_try
#define fz_do_try mu_p_fz_do_try
#define fz_do_always mu_p_fz_do_always
#define fz_do_catch mu_p_fz_do_catch
#define fz_throw mu_p_fz_throw
#define fz_caught_message mu_p_fz_caught_message
#define fz_open_document mu_p_fz_open_document
#define fz_needs_password mu_p_fz_needs_password
#define fz_authenticate_password mu_p_fz_authenticate_password
#define fz_has_permission mu_p_fz_has_permission
#define fz_count_pages mu_p_fz_count_pages
#define fz_load_page mu_p_fz_load_page
#define fz_bound_page mu_p_fz_bound_page
#define fz_drop_page mu_p_fz_drop_page
#define fz_drop_document mu_p_fz_drop_document
#define fz_page_label mu_p_fz_page_label
#define fz_scale mu_p_fz_scale
#define fz_rotate mu_p_fz_rotate
#define fz_concat mu_p_fz_concat
#define fz_translate mu_p_fz_translate
#define fz_transform_rect mu_p_fz_transform_rect
#define fz_transform_point mu_p_fz_transform_point
#define fz_round_rect mu_p_fz_round_rect
#define fz_new_pixmap mu_p_fz_new_pixmap
#define fz_clear_pixmap_with_value mu_p_fz_clear_pixmap_with_value
#define fz_new_draw_device mu_p_fz_new_draw_device
#define fz_run_page mu_p_fz_run_page
#define fz_close_device mu_p_fz_close_device
#define fz_drop_device mu_p_fz_drop_device
#define fz_drop_pixmap mu_p_fz_drop_pixmap
#define fz_device_rgb mu_p_fz_device_rgb
#define fz_device_bgr mu_p_fz_device_bgr
#define fz_pixmap_width mu_p_fz_pixmap_width
#define fz_pixmap_height mu_p_fz_pixmap_height
#define fz_pixmap_stride mu_p_fz_pixmap_stride
#define fz_pixmap_samples mu_p_fz_pixmap_samples
#define fz_save_pixmap_as_png mu_p_fz_save_pixmap_as_png
#define fz_new_stext_page_from_page mu_p_fz_new_stext_page_from_page
#define fz_drop_stext_page mu_p_fz_drop_stext_page
#define fz_search_page mu_p_fz_search_page
#define fz_snap_selection mu_p_fz_snap_selection
#define fz_highlight_selection mu_p_fz_highlight_selection
#define fz_copy_selection mu_p_fz_copy_selection
#define fz_free mu_p_fz_free
#define fz_new_buffer mu_p_fz_new_buffer
#define fz_new_output_with_buffer mu_p_fz_new_output_with_buffer
#define fz_print_stext_page_as_text mu_p_fz_print_stext_page_as_text
#define fz_close_output mu_p_fz_close_output
#define fz_drop_output mu_p_fz_drop_output
#define fz_buffer_storage mu_p_fz_buffer_storage
#define fz_drop_buffer mu_p_fz_drop_buffer

int mu_load(void);
void mu_unload(void);
int mu_ready(void);
const wchar_t *mu_error(void);

fz_context *mu_new_context(void);

char *mu_wide_to_utf8_alloc(const wchar_t *text);
wchar_t *mu_utf8_to_wide_alloc(const char *text);

#endif

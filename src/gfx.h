#ifndef PDF_LEVE_GFX_H
#define PDF_LEVE_GFX_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

int gfx_init(HWND hwnd);
void gfx_shutdown(void);
void gfx_new_frame(void);
int gfx_render(int r, int g, int b, const wchar_t *capture_path);
int gfx_resize(int width, int height);
int gfx_page_texture(const unsigned char *bgra, int width, int height, int stride);
unsigned long long gfx_page_texture_id(void);
int gfx_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

#ifdef __cplusplus
}
#endif

#endif

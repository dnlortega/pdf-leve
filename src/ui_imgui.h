#ifndef PDF_LEVE_UI_IMGUI_H
#define PDF_LEVE_UI_IMGUI_H

#include "app.h"

void ui_init(App *app);
void ui_shutdown(App *app);
void ui_frame(App *app);
void ui_show_about(void);
void ui_show_splash(void);
void ui_show_settings(void);
void ui_clear_color(const App *app, int *r, int *g, int *b);

#endif

#ifndef PDF_LEVE_GEMINI_H
#define PDF_LEVE_GEMINI_H

#include <windows.h>

#define WM_APP_GEMINI_DONE (WM_APP + 2)
#define GEMINI_KEY_MAX 256

typedef struct GeminiResult {
	int ok;
	int owner_id;
	char *text;
	char *error;
} GeminiResult;

int gemini_load_key(char *out, size_t cap);
int gemini_save_key(const char *key);
int gemini_request_async(HWND hwnd, const char *api_key, const char *prompt, int owner_id);

#endif

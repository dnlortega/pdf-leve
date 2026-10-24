#ifndef PDF_LEVE_OCR_H
#define PDF_LEVE_OCR_H

#include <windows.h>
#include <wchar.h>

#define WM_APP_OCR_DONE (WM_APP + 10)

typedef struct OcrResult {
	int page;
	int ok;
	wchar_t *text;
	wchar_t error[256];
} OcrResult;

int ocr_script_path(wchar_t *out, size_t cap);
int ocr_cache_path(const wchar_t *pdf_path, int page, wchar_t *out, size_t cap);
wchar_t *ocr_cache_read(const wchar_t *cache_path);
void ocr_temp_path(const wchar_t *suffix, wchar_t *out, size_t cap);
int ocr_launch(HWND notify, const wchar_t *image_path, const wchar_t *output_path,
	const wchar_t *cache_path, int page);

#endif

#include "ocr.h"
#include "state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

static void ensure_dir(const wchar_t *path)
{
	wchar_t buffer[1024];
	unsigned int i;
	wcsncpy(buffer, path, 1023);
	buffer[1023] = 0;
	for (i = 3; buffer[i]; i++) {
		if (buffer[i] == L'\\') {
			wchar_t saved = buffer[i];
			buffer[i] = 0;
			CreateDirectoryW(buffer, NULL);
			buffer[i] = saved;
		}
	}
	CreateDirectoryW(buffer, NULL);
}

static int exe_dir(wchar_t *out, size_t cap)
{
	wchar_t *slash;
	if (!GetModuleFileNameW(NULL, out, (DWORD)cap))
		return 0;
	slash = wcsrchr(out, L'\\');
	if (!slash)
		return 0;
	*slash = 0;
	return 1;
}

int ocr_script_path(wchar_t *out, size_t cap)
{
	wchar_t dir[900];
	wchar_t local[900];
	HRSRC res;
	DWORD size;
	HGLOBAL data;
	void *bytes;
	HANDLE file;
	DWORD written;
	HANDLE self = GetModuleHandleW(NULL);

	if (exe_dir(dir, 900)) {
		_snwprintf(out, cap - 1, L"%s\\ocr_windows.ps1", dir);
		out[cap - 1] = 0;
		if (GetFileAttributesW(out) != INVALID_FILE_ATTRIBUTES)
			return 1;
	}

	state_data_dir(local, 900);
	wcsncat(local, L"\\runtime", 900 - wcslen(local) - 1);
	ensure_dir(local);
	_snwprintf(out, cap - 1, L"%s\\ocr_windows.ps1", local);
	out[cap - 1] = 0;
	if (GetFileAttributesW(out) != INVALID_FILE_ATTRIBUTES)
		return 1;

	res = FindResourceW(self, L"OCR_SCRIPT", RT_RCDATA);
	if (!res)
		return 0;
	size = SizeofResource(self, res);
	data = LoadResource(self, res);
	if (!data || !size)
		return 0;
	bytes = LockResource(data);
	if (!bytes)
		return 0;
	file = CreateFileW(out, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return 0;
	if (!WriteFile(file, bytes, size, &written, NULL) || written != size) {
		CloseHandle(file);
		return 0;
	}
	CloseHandle(file);
	return 1;
}

static unsigned long long fnv1a(const wchar_t *text)
{
	unsigned long long hash = 1469598103934665603ULL;
	while (*text) {
		unsigned int ch = (unsigned int)towlower(*text++);
		hash ^= (unsigned char)(ch & 0xff);
		hash *= 1099511628211ULL;
		hash ^= (unsigned char)((ch >> 8) & 0xff);
		hash *= 1099511628211ULL;
	}
	return hash;
}

int ocr_cache_path(const wchar_t *pdf_path, int page, wchar_t *out, size_t cap)
{
	wchar_t dir[900];
	WIN32_FILE_ATTRIBUTE_DATA fad;
	unsigned long long hash;
	unsigned long long stamp = 0;
	if (!pdf_path || !pdf_path[0])
		return 0;
	state_data_dir(dir, 900);
	wcsncat(dir, L"\\ocr", 900 - wcslen(dir) - 1);
	ensure_dir(dir);
	hash = fnv1a(pdf_path);
	if (GetFileAttributesExW(pdf_path, GetFileExInfoStandard, &fad)) {
		stamp = ((unsigned long long)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
		stamp ^= ((unsigned long long)fad.ftLastWriteTime.dwHighDateTime << 32) | fad.ftLastWriteTime.dwLowDateTime;
	}
	_snwprintf(out, cap - 1, L"%s\\%016llx-%016llx-p%d.txt", dir, hash, stamp, page + 1);
	out[cap - 1] = 0;
	return 1;
}

static wchar_t *read_utf8_file(const wchar_t *path)
{
	FILE *file = _wfopen(path, L"rb");
	long size;
	char *buffer;
	wchar_t *wide;
	int need;
	if (!file)
		return NULL;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size < 0 || size > 32 * 1024 * 1024) {
		fclose(file);
		return NULL;
	}
	buffer = (char *)malloc((size_t)size + 1);
	if (!buffer) {
		fclose(file);
		return NULL;
	}
	if (fread(buffer, 1, (size_t)size, file) != (size_t)size) {
		free(buffer);
		fclose(file);
		return NULL;
	}
	buffer[size] = 0;
	fclose(file);
	{
		char *start = buffer;
		if (size >= 3 && (unsigned char)start[0] == 0xEF &&
			(unsigned char)start[1] == 0xBB && (unsigned char)start[2] == 0xBF)
			start += 3;
		need = MultiByteToWideChar(CP_UTF8, 0, start, -1, NULL, 0);
		if (need <= 0) {
			free(buffer);
			return NULL;
		}
		wide = (wchar_t *)malloc((size_t)need * sizeof(wchar_t));
		if (wide)
			MultiByteToWideChar(CP_UTF8, 0, start, -1, wide, need);
	}
	free(buffer);
	return wide;
}

wchar_t *ocr_cache_read(const wchar_t *cache_path)
{
	if (!cache_path || !cache_path[0])
		return NULL;
	if (GetFileAttributesW(cache_path) == INVALID_FILE_ATTRIBUTES)
		return NULL;
	return read_utf8_file(cache_path);
}

void ocr_temp_path(const wchar_t *suffix, wchar_t *out, size_t cap)
{
	wchar_t temp[900];
	GetTempPathW(900, temp);
	_snwprintf(out, cap - 1, L"%spdf-leve-%lu-%llu%s", temp,
		(unsigned long)GetCurrentProcessId(),
		(unsigned long long)GetTickCount64(), suffix ? suffix : L"");
	out[cap - 1] = 0;
}

typedef struct OcrJob {
	HWND notify;
	wchar_t image[MAX_PATH * 2];
	wchar_t output[MAX_PATH * 2];
	wchar_t cache[MAX_PATH * 2];
	int page;
} OcrJob;

static DWORD WINAPI ocr_worker(void *param)
{
	OcrJob *job = (OcrJob *)param;
	OcrResult *result;
	wchar_t script[1024] = L"";
	wchar_t powershell[900];
	wchar_t command[8192];
	wchar_t error[256] = L"";
	int ok = 0;

	result = (OcrResult *)calloc(1, sizeof(OcrResult));
	if (!result) {
		free(job);
		return 0;
	}
	result->page = job->page;

	if (!ocr_script_path(script, 1024)) {
		wcsncpy(error, L"Script de OCR não encontrado.", 255);
	} else if (!GetSystemDirectoryW(powershell, 900)) {
		wcsncpy(error, L"PowerShell do Windows não encontrado.", 255);
	} else {
		PROCESS_INFORMATION pi;
		STARTUPINFOW si;
		BOOL created;
		wcscat(powershell, L"\\WindowsPowerShell\\v1.0\\powershell.exe");
		_snwprintf(command, 8191,
			L"\"%s\" -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"%s\" "
			L"-ImagePath \"%s\" -OutputPath \"%s\" -Language pt-BR",
			powershell, script, job->image, job->output);
		command[8191] = 0;
		memset(&pi, 0, sizeof(pi));
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		created = CreateProcessW(NULL, command, NULL, NULL, FALSE,
			CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
		if (!created) {
			_snwprintf(error, 255, L"Não foi possível iniciar o OCR (erro %lu).",
				(unsigned long)GetLastError());
			error[255] = 0;
		} else {
			DWORD wait = WaitForSingleObject(pi.hProcess, 180000);
			DWORD code = 1;
			GetExitCodeProcess(pi.hProcess, &code);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			if (wait == WAIT_TIMEOUT) {
				wcsncpy(error, L"O OCR demorou demais e foi encerrado.", 255);
			} else if (code != 0) {
				wcsncpy(error, L"O Windows não conseguiu reconhecer o texto.", 255);
			} else {
				wchar_t *text = read_utf8_file(job->output);
				if (text) {
					size_t len = wcslen(text);
					if (len > 0 && (text[len - 1] == L'\n' || text[len - 1] == L'\r'))
						text[--len] = 0;
					if (len > 0)
						ok = 1;
					result->text = text;
				}
				if (!ok) {
					free(result->text);
					result->text = NULL;
					wcsncpy(error, L"Nenhum texto foi reconhecido nesta página.", 255);
				} else {
					CopyFileW(job->output, job->cache, FALSE);
				}
			}
		}
	}

	DeleteFileW(job->image);
	DeleteFileW(job->output);
	result->ok = ok;
	if (!ok)
		wcsncpy(result->error, error[0] ? error : L"Falha no OCR.", 255);
	PostMessageW(job->notify, WM_APP_OCR_DONE, 0, (LPARAM)result);
	free(job);
	return 0;
}

int ocr_launch(HWND notify, const wchar_t *image_path, const wchar_t *output_path,
	const wchar_t *cache_path, int page)
{
	OcrJob *job;
	HANDLE thread;
	job = (OcrJob *)calloc(1, sizeof(OcrJob));
	if (!job)
		return 0;
	job->notify = notify;
	wcsncpy(job->image, image_path, MAX_PATH * 2 - 1);
	wcsncpy(job->output, output_path, MAX_PATH * 2 - 1);
	wcsncpy(job->cache, cache_path, MAX_PATH * 2 - 1);
	job->page = page;
	thread = CreateThread(NULL, 0, ocr_worker, job, 0, NULL);
	if (!thread) {
		free(job);
		return 0;
	}
	CloseHandle(thread);
	return 1;
}

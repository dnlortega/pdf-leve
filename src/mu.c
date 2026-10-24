#include "mu_symbols.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MU_DEF(name) __typeof__(name) *mu_p_##name;
MU_FUNCS(MU_DEF)
#undef MU_DEF

static HMODULE g_module;
static int g_ready;
static wchar_t g_error[512] = L"MuPDF não carregado";

void mu_unload(void);

int mu_ready(void)
{
	return g_ready;
}

const wchar_t *mu_error(void)
{
	return g_error;
}

static void set_error(const wchar_t *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	_vsnwprintf(g_error, 511, fmt, ap);
	va_end(ap);
	g_error[511] = 0;
}

static int write_resource_to_file(HMODULE module, const wchar_t *name, const wchar_t *path)
{
	HRSRC res = FindResourceW(module, name, RT_RCDATA);
	DWORD size;
	HGLOBAL data;
	void *bytes;
	HANDLE file;
	DWORD written = 0;
	if (!res)
		return 0;
	size = SizeofResource(module, res);
	data = LoadResource(module, res);
	if (!data || size == 0)
		return 0;
	bytes = LockResource(data);
	if (!bytes)
		return 0;
	file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return 0;
	if (!WriteFile(file, bytes, size, &written, NULL) || written != size) {
		CloseHandle(file);
		DeleteFileW(path);
		return 0;
	}
	CloseHandle(file);
	return 1;
}

static int ensure_directory(const wchar_t *path)
{
	wchar_t buffer[1024];
	unsigned int i;
	wcscpy(buffer, path);
	for (i = 3; buffer[i]; i++) {
		if (buffer[i] == L'\\' || buffer[i] == L'/') {
			wchar_t saved = buffer[i];
			buffer[i] = 0;
			CreateDirectoryW(buffer, NULL);
			buffer[i] = saved;
		}
	}
	CreateDirectoryW(buffer, NULL);
	return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
}

static int runtime_current(const wchar_t *target)
{
	HMODULE self = GetModuleHandleW(NULL);
	HRSRC res = FindResourceW(self, L"MUPDF_DLL", RT_RCDATA);
	WIN32_FILE_ATTRIBUTE_DATA fad;
	DWORD size;
	if (!res)
		return 0;
	size = SizeofResource(self, res);
	if (!size || !GetFileAttributesExW(target, GetFileExInfoStandard, &fad))
		return 0;
	return fad.nFileSizeLow == size;
}

static int ensure_dependencies(const wchar_t *target)
{
	HMODULE self = GetModuleHandleW(NULL);
	static const wchar_t *deps[] = { L"MSVCP140_DLL", L"VCRUNTIME140_DLL", L"VCRUNTIME140_1_DLL" };
	static const wchar_t *files[] = { L"\\MSVCP140.dll", L"\\VCRUNTIME140.dll", L"\\VCRUNTIME140_1.dll" };
	wchar_t dir[1024];
	wchar_t *slash;
	int i;

	wcscpy(dir, target);
	slash = wcsrchr(dir, L'\\');
	if (slash)
		*slash = 0;
	for (i = 0; i < 3; i++) {
		wchar_t dep[1024];
		wcscpy(dep, dir);
		wcscat(dep, files[i]);
		if (GetFileAttributesW(dep) == INVALID_FILE_ATTRIBUTES) {
			if (!write_resource_to_file(self, deps[i], dep))
				return 0;
		}
	}
	return 1;
}

static int extract_runtime(const wchar_t *target)
{
	HMODULE self = GetModuleHandleW(NULL);

	if (!write_resource_to_file(self, L"MUPDF_DLL", target))
		return 0;
	return ensure_dependencies(target);
}

static HMODULE load_from(const wchar_t *path)
{
	return LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
}

static HMODULE open_module(void)
{
	wchar_t exe[1024];
	wchar_t dll[1024];
	wchar_t local[1024];
	wchar_t *slash;
	HMODULE module;
	DWORD attr;

	GetModuleFileNameW(NULL, exe, 1024);
	wcscpy(dll, exe);
	slash = wcsrchr(dll, L'\\');
	if (!slash)
		return NULL;
	wcscpy(slash + 1, L"mupdfcpp64.dll");
	if (GetFileAttributesW(dll) != INVALID_FILE_ATTRIBUTES) {
		module = load_from(dll);
		if (module)
			return module;
	}

	GetEnvironmentVariableW(L"LOCALAPPDATA", local, 1024);
	if (!local[0])
		return NULL;
	wcscat(local, L"\\PDF Leve\\runtime");
	if (!ensure_directory(local))
		return NULL;
	wcscat(local, L"\\mupdfcpp64.dll");
	attr = GetFileAttributesW(local);
	if (attr == INVALID_FILE_ATTRIBUTES || !runtime_current(local)) {
		DeleteFileW(local);
		if (!extract_runtime(local))
			return NULL;
	} else if (!ensure_dependencies(local)) {
		return NULL;
	}
	module = load_from(local);
	if (module)
		return module;

	DeleteFileW(local);
	if (!extract_runtime(local))
		return NULL;
	return load_from(local);
}

int mu_load(void)
{
	HMODULE module;
	if (g_ready)
		return 1;
	module = open_module();
	if (!module) {
		set_error(L"Não foi possível carregar o motor MuPDF (mupdfcpp64.dll).");
		return 0;
	}
	g_module = module;

#define MU_RESOLVE(name) \
	mu_p_##name = (__typeof__(mu_p_##name))GetProcAddress(module, #name); \
	if (!mu_p_##name) { \
		set_error(L"MuPDF sem o símbolo %hs.", #name); \
		mu_unload(); \
		return 0; \
	}
	MU_FUNCS(MU_RESOLVE)
#undef MU_RESOLVE

	g_ready = 1;
	g_error[0] = 0;
	return 1;
}

void mu_unload(void)
{
	if (g_module) {
		FreeLibrary(g_module);
		g_module = NULL;
	}
	g_ready = 0;
}

fz_context *mu_new_context(void)
{
	fz_context *ctx;
	if (!g_ready)
		return NULL;
	ctx = mu_p_fz_new_context_imp(NULL, NULL, FZ_STORE_DEFAULT, FZ_VERSION);
	if (ctx)
		mu_p_fz_register_document_handlers(ctx);
	return ctx;
}

char *mu_wide_to_utf8_alloc(const wchar_t *text)
{
	int need;
	char *out;
	if (!text)
		return NULL;
	need = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
	if (need <= 0)
		return NULL;
	out = (char *)malloc((size_t)need);
	if (!out)
		return NULL;
	if (!WideCharToMultiByte(CP_UTF8, 0, text, -1, out, need, NULL, NULL)) {
		free(out);
		return NULL;
	}
	return out;
}

wchar_t *mu_utf8_to_wide_alloc(const char *text)
{
	int need;
	wchar_t *out;
	if (!text)
		return NULL;
	need = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	if (need <= 0)
		return NULL;
	out = (wchar_t *)malloc((size_t)need * sizeof(wchar_t));
	if (!out)
		return NULL;
	if (!MultiByteToWideChar(CP_UTF8, 0, text, -1, out, need)) {
		free(out);
		return NULL;
	}
	return out;
}

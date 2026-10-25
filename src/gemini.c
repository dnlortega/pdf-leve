#include "gemini.h"
#include "state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winhttp.h>
#include <wincrypt.h>

typedef struct StrBuf {
	char *data;
	size_t len;
	size_t cap;
} StrBuf;

typedef struct GeminiRequest {
	HWND hwnd;
	int owner_id;
	char *key;
	char *body;
} GeminiRequest;

static char *wide_to_utf8_dup(const wchar_t *text)
{
	char *out;
	int need;
	if (!text)
		return NULL;
	need = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
	if (need <= 0)
		return NULL;
	out = (char *)malloc((size_t)need);
	if (!out)
		return NULL;
	WideCharToMultiByte(CP_UTF8, 0, text, -1, out, need, NULL, NULL);
	return out;
}

static void gemini_key_path(wchar_t *out, size_t cap)
{
	wchar_t dir[1024];
	state_config_dir(dir, 1024);
	_snwprintf(out, cap - 1, L"%s\\gemini.bin", dir);
	out[cap - 1] = 0;
}

int gemini_load_key(char *out, size_t cap)
{
	wchar_t path[1024];
	FILE *file;
	long size;
	BYTE *data;
	DATA_BLOB input;
	DATA_BLOB plain;
	DWORD len;

	if (!out || cap < 2)
		return 0;
	out[0] = 0;
	gemini_key_path(path, 1024);
	file = _wfopen(path, L"rb");
	if (!file)
		return 0;
	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return 0;
	}
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size <= 0 || size > 8192) {
		fclose(file);
		return 0;
	}
	data = (BYTE *)malloc((size_t)size);
	if (!data) {
		fclose(file);
		return 0;
	}
	if (fread(data, 1, (size_t)size, file) != (size_t)size) {
		free(data);
		fclose(file);
		return 0;
	}
	fclose(file);
	input.pbData = data;
	input.cbData = (DWORD)size;
	memset(&plain, 0, sizeof(plain));
	if (!CryptUnprotectData(&input, NULL, NULL, NULL, NULL,
		CRYPTPROTECT_UI_FORBIDDEN, &plain)) {
		free(data);
		return 0;
	}
	free(data);
	len = plain.cbData;
	if (len > 0 && plain.pbData[len - 1] == 0)
		len--;
	if (len >= cap)
		len = (DWORD)cap - 1;
	memcpy(out, plain.pbData, len);
	out[len] = 0;
	LocalFree(plain.pbData);
	if (strchr(out, '\r') || strchr(out, '\n')) {
		out[0] = 0;
		return 0;
	}
	return (int)strlen(out);
}

int gemini_save_key(const char *key)
{
	wchar_t path[1024];
	wchar_t temp[1024];
	FILE *file;
	DATA_BLOB input;
	DATA_BLOB crypt;
	size_t len;

	gemini_key_path(path, 1024);
	if (!key || !key[0]) {
		DeleteFileW(path);
		return 1;
	}
	len = strlen(key);
	if (len + 1 > GEMINI_KEY_MAX || strchr(key, '\r') || strchr(key, '\n'))
		return 0;
	input.pbData = (BYTE *)key;
	input.cbData = (DWORD)(len + 1);
	memset(&crypt, 0, sizeof(crypt));
	if (!CryptProtectData(&input, L"PDF Leve Gemini", NULL, NULL, NULL,
		CRYPTPROTECT_UI_FORBIDDEN, &crypt))
		return 0;
	wcscpy(temp, path);
	wcscat(temp, L".tmp");
	file = _wfopen(temp, L"wb");
	if (!file) {
		LocalFree(crypt.pbData);
		return 0;
	}
	if (fwrite(crypt.pbData, 1, crypt.cbData, file) != crypt.cbData) {
		fclose(file);
		LocalFree(crypt.pbData);
		DeleteFileW(temp);
		return 0;
	}
	fclose(file);
	LocalFree(crypt.pbData);
	if (!MoveFileExW(temp, path, MOVEFILE_REPLACE_EXISTING)) {
		DeleteFileW(temp);
		return 0;
	}
	return 1;
}

static int sb_reserve(StrBuf *buf, size_t extra)
{
	size_t need;
	size_t cap;
	char *data;

	if (!buf)
		return 0;
	need = buf->len + extra + 1;
	if (need <= buf->cap)
		return 1;
	cap = buf->cap ? buf->cap : 1024;
	while (cap < need)
		cap *= 2;
	data = (char *)realloc(buf->data, cap);
	if (!data)
		return 0;
	buf->data = data;
	buf->cap = cap;
	return 1;
}

static int sb_add(StrBuf *buf, const char *text, size_t len)
{
	if (!sb_reserve(buf, len))
		return 0;
	memcpy(buf->data + buf->len, text, len);
	buf->len += len;
	buf->data[buf->len] = 0;
	return 1;
}

static int sb_addc(StrBuf *buf, char c)
{
	return sb_add(buf, &c, 1);
}

static int sb_add_utf8(StrBuf *buf, unsigned int cp)
{
	char out[4];
	int len;

	if (cp < 0x80) {
		out[0] = (char)cp;
		len = 1;
	} else if (cp < 0x800) {
		out[0] = (char)(0xC0 | (cp >> 6));
		out[1] = (char)(0x80 | (cp & 0x3F));
		len = 2;
	} else if (cp < 0x10000) {
		out[0] = (char)(0xE0 | (cp >> 12));
		out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[2] = (char)(0x80 | (cp & 0x3F));
		len = 3;
	} else {
		out[0] = (char)(0xF0 | (cp >> 18));
		out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
		out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[3] = (char)(0x80 | (cp & 0x3F));
		len = 4;
	}
	return sb_add(buf, out, (size_t)len);
}

static int json_escape(StrBuf *buf, const char *text)
{
	const unsigned char *p = (const unsigned char *)text;
	while (*p) {
		switch (*p) {
		case '"':
			if (!sb_add(buf, "\\\"", 2))
				return 0;
			break;
		case '\\':
			if (!sb_add(buf, "\\\\", 2))
				return 0;
			break;
		case '\n':
			if (!sb_add(buf, "\\n", 2))
				return 0;
			break;
		case '\r':
			if (!sb_add(buf, "\\r", 2))
				return 0;
			break;
		case '\t':
			if (!sb_add(buf, "\\t", 2))
				return 0;
			break;
		case '\b':
			if (!sb_add(buf, "\\b", 2))
				return 0;
			break;
		case '\f':
			if (!sb_add(buf, "\\f", 2))
				return 0;
			break;
		default:
			if (*p < 0x20) {
				char esc[7];
				snprintf(esc, sizeof(esc), "\\u%04x", (unsigned int)*p);
				if (!sb_add(buf, esc, 6))
					return 0;
			} else if (!sb_addc(buf, (char)*p)) {
				return 0;
			}
			break;
		}
		p++;
	}
	return 1;
}

static int hex4(const char *p, unsigned int *out)
{
	unsigned int value = 0;
	int i;
	for (i = 0; i < 4; i++) {
		char c = p[i];
		value <<= 4;
		if (c >= '0' && c <= '9')
			value |= (unsigned int)(c - '0');
		else if (c >= 'a' && c <= 'f')
			value |= (unsigned int)(c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			value |= (unsigned int)(c - 'A' + 10);
		else
			return 0;
	}
	*out = value;
	return 1;
}

static const char *json_parse_string(const char *p, StrBuf *out)
{
	if (*p != '"')
		return NULL;
	p++;
	while (*p && *p != '"') {
		if (*p == '\\') {
			p++;
			switch (*p) {
			case '"':
				if (!sb_addc(out, '"'))
					return NULL;
				p++;
				break;
			case '\\':
				if (!sb_addc(out, '\\'))
					return NULL;
				p++;
				break;
			case '/':
				if (!sb_addc(out, '/'))
					return NULL;
				p++;
				break;
			case 'b':
				if (!sb_addc(out, '\b'))
					return NULL;
				p++;
				break;
			case 'f':
				if (!sb_addc(out, '\f'))
					return NULL;
				p++;
				break;
			case 'n':
				if (!sb_addc(out, '\n'))
					return NULL;
				p++;
				break;
			case 'r':
				if (!sb_addc(out, '\r'))
					return NULL;
				p++;
				break;
			case 't':
				if (!sb_addc(out, '\t'))
					return NULL;
				p++;
				break;
			case 'u': {
				unsigned int cp;
				if (!hex4(p + 1, &cp))
					return NULL;
				p += 5;
				if (cp >= 0xD800 && cp <= 0xDBFF &&
					p[0] == '\\' && p[1] == 'u') {
					unsigned int low;
					if (hex4(p + 2, &low) && low >= 0xDC00 && low <= 0xDFFF) {
						cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
						p += 6;
					}
				}
				if (!sb_add_utf8(out, cp))
					return NULL;
				break;
			}
			default:
				return NULL;
			}
		} else {
			if (!sb_addc(out, *p))
				return NULL;
			p++;
		}
	}
	if (*p != '"')
		return NULL;
	return p + 1;
}

static int collect_json_field(const char *json, const char *field, StrBuf *out)
{
	const char *p = json;
	size_t field_len = strlen(field);
	int found = 0;

	while ((p = strstr(p, field)) != NULL) {
		const char *q;
		if (p > json && p[-1] == '\\') {
			p += field_len;
			continue;
		}
		q = p + field_len;
		while (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n')
			q++;
		if (*q != ':') {
			p = q;
			continue;
		}
		q++;
		while (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n')
			q++;
		if (*q != '"') {
			p = q;
			continue;
		}
		q = json_parse_string(q, out);
		if (!q)
			break;
		found++;
		p = q;
	}
	return found;
}

static char *win_error_utf8(DWORD code, const wchar_t *prefix)
{
	wchar_t *system_message = NULL;
	wchar_t message[512];
	char *utf8;

	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, 0,
		(LPWSTR)&system_message, 0, NULL);
	_snwprintf(message, 511, L"%s: %s", prefix,
		system_message ? system_message : L"falha de rede");
	message[511] = 0;
	if (system_message)
		LocalFree(system_message);
	utf8 = wide_to_utf8_dup(message);
	return utf8;
}

static void gemini_model_path(wchar_t *out, size_t cap)
{
	wchar_t model[128];
	DWORD len = GetEnvironmentVariableW(L"PDFLEVE_GEMINI_MODEL", model, 128);
	if (len == 0 || len >= 128)
		wcscpy(model, L"gemini-2.5-flash");
	_snwprintf(out, cap - 1, L"/v1beta/models/%s:generateContent", model);
	out[cap - 1] = 0;
}

static int http_post(const char *key, const char *body, char **response,
	DWORD *status_out, char **error_out)
{
	wchar_t key_w[512];
	wchar_t path[256];
	wchar_t headers[1024];
	HINTERNET session = NULL;
	HINTERNET connection = NULL;
	HINTERNET request = NULL;
	StrBuf buffer;
	DWORD status = 0;
	DWORD size;
	BOOL ok;
	int result = 0;

	*response = NULL;
	*error_out = NULL;
	*status_out = 0;
	if (MultiByteToWideChar(CP_UTF8, 0, key, -1, key_w, 512) <= 0) {
		*error_out = wide_to_utf8_dup(L"Chave de API inválida.");
		return 0;
	}
	_snwprintf(headers, 1023,
		L"Content-Type: application/json\r\nx-goog-api-key: %s", key_w);
	gemini_model_path(path, 256);
	memset(&buffer, 0, sizeof(buffer));

	session = WinHttpOpen(L"PDF Leve/1.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session) {
		*error_out = win_error_utf8(GetLastError(), L"Não foi possível iniciar o WinHTTP");
		goto done;
	}
	WinHttpSetTimeouts(session, 20000, 20000, 30000, 120000);
	connection = WinHttpConnect(session, L"generativelanguage.googleapis.com",
		INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (!connection) {
		*error_out = win_error_utf8(GetLastError(), L"Não foi possível conectar ao Google");
		goto done;
	}
	request = WinHttpOpenRequest(connection, L"POST", path, NULL,
		WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	if (!request) {
		*error_out = win_error_utf8(GetLastError(), L"Não foi possível criar a requisição");
		goto done;
	}
	ok = WinHttpSendRequest(request, headers, (DWORD)-1L, (LPVOID)body,
		(DWORD)strlen(body), (DWORD)strlen(body), 0);
	if (!ok) {
		*error_out = win_error_utf8(GetLastError(), L"Falha ao enviar a consulta");
		goto done;
	}
	ok = WinHttpReceiveResponse(request, NULL);
	if (!ok) {
		*error_out = win_error_utf8(GetLastError(), L"Falha ao receber a resposta");
		goto done;
	}
	size = sizeof(status);
	WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
	for (;;) {
		DWORD available = 0;
		DWORD read = 0;
		if (!WinHttpQueryDataAvailable(request, &available)) {
			*error_out = win_error_utf8(GetLastError(), L"Falha ao ler a resposta");
			goto done;
		}
		if (available == 0)
			break;
		if (buffer.len + available > 4u * 1024u * 1024u) {
			*error_out = wide_to_utf8_dup(L"A resposta do Gemini é grande demais.");
			goto done;
		}
		if (!sb_reserve(&buffer, available)) {
			*error_out = wide_to_utf8_dup(L"Memória insuficiente para a resposta.");
			goto done;
		}
		if (!WinHttpReadData(request, buffer.data + buffer.len, available, &read)) {
			*error_out = win_error_utf8(GetLastError(), L"Falha ao ler a resposta");
			goto done;
		}
		if (read == 0)
			break;
		buffer.len += read;
		buffer.data[buffer.len] = 0;
	}
	if (!buffer.data && !sb_reserve(&buffer, 0)) {
		*error_out = wide_to_utf8_dup(L"Memória insuficiente para a resposta.");
		goto done;
	}
	*status_out = status;
	*response = buffer.data;
	buffer.data = NULL;
	result = 1;

done:
	free(buffer.data);
	if (request)
		WinHttpCloseHandle(request);
	if (connection)
		WinHttpCloseHandle(connection);
	if (session)
		WinHttpCloseHandle(session);
	return result;
}

static void result_set_error(GeminiResult *result, const char *text)
{
	free(result->error);
	result->error = text ? _strdup(text) : NULL;
	if (!result->error)
		result->error = _strdup("Falha ao consultar o Gemini.");
}

static void result_from_response(GeminiResult *result, const char *json, DWORD status)
{
	StrBuf text;
	StrBuf message;

	memset(&text, 0, sizeof(text));
	memset(&message, 0, sizeof(message));
	collect_json_field(json, "\"message\"", &message);
	if (status == 200) {
		collect_json_field(json, "\"text\"", &text);
		if (text.len > 0) {
			result->ok = 1;
			result->text = text.data;
			text.data = NULL;
		} else {
			StrBuf reason;
			char fallback[256];
			memset(&reason, 0, sizeof(reason));
			collect_json_field(json, "\"blockReason\"", &reason);
			if (message.len > 0) {
				result_set_error(result, message.data);
			} else if (reason.len > 0) {
				snprintf(fallback, sizeof(fallback),
					"O Gemini bloqueou a resposta (%s).", reason.data);
				result_set_error(result, fallback);
			} else {
				result_set_error(result, "O Gemini não retornou texto.");
			}
			free(reason.data);
		}
	} else {
		char fallback[512];
		if (message.len > 0) {
			snprintf(fallback, sizeof(fallback), "Gemini (HTTP %lu): %s",
				(unsigned long)status, message.data);
			result_set_error(result, fallback);
		} else {
			snprintf(fallback, sizeof(fallback),
				"O Gemini respondeu com HTTP %lu.", (unsigned long)status);
			result_set_error(result, fallback);
		}
	}
	free(text.data);
	free(message.data);
}

static DWORD WINAPI gemini_worker(LPVOID parameter)
{
	GeminiRequest *request = (GeminiRequest *)parameter;
	GeminiResult *result = (GeminiResult *)calloc(1, sizeof(GeminiResult));
	HWND hwnd = request->hwnd;
	char *response = NULL;
	char *error = NULL;
	DWORD status = 0;

	if (result) {
		result->owner_id = request->owner_id;
		if (http_post(request->key, request->body, &response, &status, &error)) {
			result_from_response(result, response ? response : "", status);
		} else {
			result->error = error;
			error = NULL;
		}
	}
	free(response);
	free(error);
	free(request->key);
	free(request->body);
	free(request);
	if (!result)
		return 0;
	if (!PostMessageW(hwnd, WM_APP_GEMINI_DONE, 0, (LPARAM)result)) {
		free(result->text);
		free(result->error);
		free(result);
	}
	return 0;
}

int gemini_request_async(HWND hwnd, const char *api_key, const char *prompt, int owner_id)
{
	StrBuf body;
	GeminiRequest *request;
	HANDLE thread;
	size_t i;

	if (!hwnd || !api_key || !api_key[0] || !prompt)
		return 0;
	for (i = 0; api_key[i]; i++) {
		unsigned char c = (unsigned char)api_key[i];
		if (c < 0x20 || c > 0x7E || api_key[i] == '\r' || api_key[i] == '\n')
			return 0;
	}
	memset(&body, 0, sizeof(body));
	{
		static const char prefix[] =
			"{\"contents\":[{\"role\":\"user\",\"parts\":[{\"text\":\"";
		static const char suffix[] =
			"\"}]}],\"generationConfig\":{\"temperature\":0.2,"
			"\"maxOutputTokens\":2048}}";
		if (!sb_add(&body, prefix, strlen(prefix)) ||
			!json_escape(&body, prompt) ||
			!sb_add(&body, suffix, strlen(suffix))) {
			free(body.data);
			return 0;
		}
	}
	request = (GeminiRequest *)calloc(1, sizeof(GeminiRequest));
	if (!request) {
		free(body.data);
		return 0;
	}
	request->hwnd = hwnd;
	request->owner_id = owner_id;
	request->key = _strdup(api_key);
	request->body = body.data;
	if (!request->key) {
		free(request->key);
		free(request->body);
		free(request);
		return 0;
	}
	thread = CreateThread(NULL, 0, gemini_worker, request, 0, NULL);
	if (!thread) {
		free(request->key);
		free(request->body);
		free(request);
		return 0;
	}
	CloseHandle(thread);
	return 1;
}

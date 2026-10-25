#include "state.h"
#include "doc.h"
#include "mu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void state_config_dir(wchar_t *out, size_t cap)
{
	out[0] = 0;
	GetEnvironmentVariableW(L"APPDATA", out, (DWORD)cap);
	if (!out[0]) {
		wcsncpy(out, L".", cap - 1);
		out[cap - 1] = 0;
		return;
	}
	wcsncat(out, L"\\PDF Leve", cap - wcslen(out) - 1);
	ensure_dir(out);
}

void state_data_dir(wchar_t *out, size_t cap)
{
	out[0] = 0;
	GetEnvironmentVariableW(L"LOCALAPPDATA", out, (DWORD)cap);
	if (!out[0]) {
		wcsncpy(out, L".", cap - 1);
		out[cap - 1] = 0;
		return;
	}
	wcsncat(out, L"\\PDF Leve", cap - wcslen(out) - 1);
	ensure_dir(out);
}

static void state_path(wchar_t *out, size_t cap)
{
	state_config_dir(out, cap);
	wcsncat(out, L"\\state.txt", cap - wcslen(out) - 1);
}

void state_format_time(long long used, wchar_t *out, size_t cap)
{
	time_t now = time(NULL);
	long long diff = (long long)now - used;
	struct tm tmv;
	time_t stamp;
	if (used <= 0) {
		wcsncpy(out, L"", cap - 1);
		out[cap - 1] = 0;
		return;
	}
	if (diff < 60) {
		wcsncpy(out, L"agora", cap - 1);
		out[cap - 1] = 0;
		return;
	}
	if (diff < 3600) {
		_snwprintf(out, cap - 1, L"há %d min", (int)(diff / 60));
		out[cap - 1] = 0;
		return;
	}
	if (diff < 86400) {
		_snwprintf(out, cap - 1, L"há %d h", (int)(diff / 3600));
		out[cap - 1] = 0;
		return;
	}
	if (diff < 172800) {
		wcsncpy(out, L"ontem", cap - 1);
		out[cap - 1] = 0;
		return;
	}
	stamp = (time_t)used;
	localtime_s(&tmv, &stamp);
	_snwprintf(out, cap - 1, L"%02d/%02d/%04d", tmv.tm_mday, tmv.tm_mon + 1, tmv.tm_year + 1900);
	out[cap - 1] = 0;
}

static int split_recent(const char *line, Recent *recent)
{
	char temp[2048];
	const char *cursor;
	char *p;
	memset(recent, 0, sizeof(*recent));
	/* path may contain '|'; split the separators from the right */
	strncpy(temp, line, sizeof(temp) - 1);
	temp[sizeof(temp) - 1] = 0;
	p = strrchr(temp, '|');
	if (!p)
		return 0;
	recent->used = _atoi64(p + 1);
	*p = 0;
	p = strrchr(temp, '|');
	if (!p)
		return 0;
	recent->rotation = atoi(p + 1);
	*p = 0;
	p = strrchr(temp, '|');
	if (!p)
		return 0;
	recent->fit = atoi(p + 1);
	*p = 0;
	p = strrchr(temp, '|');
	if (!p)
		return 0;
	recent->zoom = (float)atof(p + 1);
	*p = 0;
	p = strrchr(temp, '|');
	if (!p)
		return 0;
	recent->page = atoi(p + 1);
	*p = 0;
	cursor = temp;
	{
		wchar_t *wide = mu_utf8_to_wide_alloc(cursor);
		if (wide) {
			wcsncpy(recent->path, wide, 1023);
			recent->path[1023] = 0;
			free(wide);
		}
	}
	return recent->path[0] != 0;
}

static int parse_state(const char *text, State *state)
{
	char line[4096];
	const char *p = text;
	int got_recent = 0;
	while (*p) {
		size_t len = 0;
		while (p[len] && p[len] != '\n' && p[len] != '\r')
			len++;
		if (len >= sizeof(line))
			len = sizeof(line) - 1;
		memcpy(line, p, len);
		line[len] = 0;
		p += len;
		while (*p == '\r' || *p == '\n')
			p++;
		if (strncmp(line, "last_file=", 10) == 0) {
			wchar_t *wide = mu_utf8_to_wide_alloc(line + 10);
			if (wide) {
				wcsncpy(state->last_file, wide, 1023);
				state->last_file[1023] = 0;
				free(wide);
			}
		} else if (strncmp(line, "last_page=", 10) == 0) {
			state->last_page = atoi(line + 10);
		} else if (strncmp(line, "zoom=", 5) == 0) {
			state->last_zoom = (float)atof(line + 5);
		} else if (strncmp(line, "fit=", 4) == 0) {
			state->last_fit = atoi(line + 4);
		} else if (strncmp(line, "rotation=", 9) == 0) {
			state->last_rotation = atoi(line + 9);
		} else if (strncmp(line, "window=", 7) == 0) {
			int l, t, w, h;
			if (sscanf(line + 7, "%d,%d,%d,%d", &l, &t, &w, &h) == 4) {
				state->window.left = l;
				state->window.top = t;
				state->window.right = l + w;
				state->window.bottom = t + h;
			}
		} else if (strncmp(line, "maximized=", 10) == 0) {
			state->maximized = atoi(line + 10);
		} else if (strncmp(line, "dark=", 5) == 0) {
			state->dark = atoi(line + 5) ? 1 : 0;
		} else if (strncmp(line, "sidebar=", 8) == 0) {
			state->sidebar = atoi(line + 8) ? 1 : 0;
		} else if (strncmp(line, "ui_scale=", 9) == 0) {
			state->ui_scale = atoi(line + 9);
		} else if (strncmp(line, "text_size=", 10) == 0) {
			state->text_size = atoi(line + 10);
		} else if (strncmp(line, "recent=", 7) == 0) {
			if (state->recent_count < MAX_RECENTS) {
				if (split_recent(line + 7, &state->recents[state->recent_count]))
					state->recent_count++;
				got_recent = 1;
			}
		}
	}
	(void)got_recent;
	return 1;
}

void state_load(State *state)
{
	wchar_t path[1024];
	FILE *file;
	long size;
	char *data;
	memset(state, 0, sizeof(*state));
	state->last_zoom = 1.0f;
	state->last_fit = FIT_WIDTH;
	state->window.right = 0;
	state->window.bottom = 0;
	state->sidebar = 1;
	state->ui_scale = 100;
	state->text_size = 17;
	state_path(path, 1024);
	file = _wfopen(path, L"rb");
	if (!file)
		return;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size <= 0 || size > 1024 * 1024) {
		fclose(file);
		return;
	}
	data = (char *)malloc((size_t)size + 1);
	if (!data) {
		fclose(file);
		return;
	}
	if (fread(data, 1, (size_t)size, file) != (size_t)size) {
		free(data);
		fclose(file);
		return;
	}
	data[size] = 0;
	fclose(file);
	parse_state(data, state);
	free(data);
	if (state->last_zoom < MIN_ZOOM || state->last_zoom > MAX_ZOOM)
		state->last_zoom = 1.0f;
	if (state->last_fit < 0 || state->last_fit > FIT_PAGE)
		state->last_fit = FIT_WIDTH;
	state->last_rotation = ((state->last_rotation % 360) + 360) % 360;
	if (state->ui_scale != 110 && state->ui_scale != 125)
		state->ui_scale = 100;
	if (state->text_size != 15 && state->text_size != 17 &&
		state->text_size != 19 && state->text_size != 21)
		state->text_size = 17;
}

static void write_recent(FILE *file, const Recent *recent)
{
	wchar_t clean[1024];
	char *utf8;
	int i;
	wcsncpy(clean, recent->path, 1023);
	clean[1023] = 0;
	for (i = 0; clean[i]; i++) {
		if (clean[i] == L'\n' || clean[i] == L'\r')
			clean[i] = L' ';
	}
	utf8 = mu_wide_to_utf8_alloc(clean);
	if (!utf8)
		return;
	fprintf(file, "recent=%s|%d|%.3f|%d|%d|%lld\n", utf8, recent->page, recent->zoom,
		recent->fit, recent->rotation, recent->used);
	free(utf8);
}

void state_save(const State *state)
{
	wchar_t path[1024];
	wchar_t temp[1024];
	FILE *file;
	char *utf8;
	int i;

	state_path(path, 1024);
	wcscpy(temp, path);
	wcscat(temp, L".tmp");
	file = _wfopen(temp, L"wb");
	if (!file)
		return;
	fprintf(file, "v=1\n");
	if (state->last_file[0]) {
		utf8 = mu_wide_to_utf8_alloc(state->last_file);
		if (utf8) {
			fprintf(file, "last_file=%s\n", utf8);
			free(utf8);
		}
		fprintf(file, "last_page=%d\n", state->last_page);
		fprintf(file, "zoom=%.3f\n", state->last_zoom);
		fprintf(file, "fit=%d\n", state->last_fit);
		fprintf(file, "rotation=%d\n", state->last_rotation);
	}
	fprintf(file, "window=%ld,%ld,%ld,%ld\n",
		(long)state->window.left, (long)state->window.top,
		(long)(state->window.right - state->window.left),
		(long)(state->window.bottom - state->window.top));
	fprintf(file, "maximized=%d\n", state->maximized);
	fprintf(file, "dark=%d\n", state->dark ? 1 : 0);
	fprintf(file, "sidebar=%d\n", state->sidebar ? 1 : 0);
	fprintf(file, "ui_scale=%d\n", state->ui_scale);
	fprintf(file, "text_size=%d\n", state->text_size);
	for (i = 0; i < state->recent_count; i++)
		write_recent(file, &state->recents[i]);
	fclose(file);
	MoveFileExW(temp, path, MOVEFILE_REPLACE_EXISTING);
}

int state_find(const State *state, const wchar_t *path)
{
	int i;
	for (i = 0; i < state->recent_count; i++) {
		if (_wcsicmp(state->recents[i].path, path) == 0)
			return i;
	}
	return -1;
}

void state_touch(State *state, const wchar_t *path, int page, float zoom, int fit, int rotation)
{
	int index = state_find(state, path);
	Recent item;
	int i;
	memset(&item, 0, sizeof(item));
	wcsncpy(item.path, path, 1023);
	item.path[1023] = 0;
	item.page = page;
	item.zoom = zoom;
	item.fit = fit;
	item.rotation = rotation;
	item.used = (long long)time(NULL);
	if (index >= 0) {
		for (i = index; i > 0; i--)
			state->recents[i] = state->recents[i - 1];
		state->recents[0] = item;
	} else {
		int count = state->recent_count;
		if (count < MAX_RECENTS)
			state->recent_count++;
		for (i = state->recent_count - 1; i > 0; i--)
			state->recents[i] = state->recents[i - 1];
		state->recents[0] = item;
	}
}

void state_set_last(State *state, const wchar_t *path, int page, float zoom, int fit, int rotation)
{
	wcsncpy(state->last_file, path, 1023);
	state->last_file[1023] = 0;
	state->last_page = page;
	state->last_zoom = zoom;
	state->last_fit = fit;
	state->last_rotation = rotation;
}

void state_clear_last(State *state)
{
	state->last_file[0] = 0;
	state->last_page = 0;
}

#ifndef PDF_LEVE_STATE_H
#define PDF_LEVE_STATE_H

#include <windows.h>
#include <time.h>
#include <wchar.h>

#define MAX_RECENTS 8

typedef struct Recent {
	wchar_t path[1024];
	int page;
	float zoom;
	int fit;
	int rotation;
	long long used;
} Recent;

typedef struct State {
	Recent recents[MAX_RECENTS];
	int recent_count;

	wchar_t last_file[1024];
	int last_page;
	float last_zoom;
	int last_fit;
	int last_rotation;

	RECT window;
	int maximized;
	int dark;
} State;

void state_load(State *state);
void state_save(const State *state);
int state_find(const State *state, const wchar_t *path);
void state_touch(State *state, const wchar_t *path, int page, float zoom, int fit, int rotation);
void state_set_last(State *state, const wchar_t *path, int page, float zoom, int fit, int rotation);
void state_clear_last(State *state);
void state_format_time(long long used, wchar_t *out, size_t cap);
void state_config_dir(wchar_t *out, size_t cap);
void state_data_dir(wchar_t *out, size_t cap);

#endif

#include <windows.h>
#include <stdio.h>
int main(void) {
    HMODULE h = LoadLibraryW(L"mupdfcpp64.dll");
    if (!h) { printf("LoadLibrary failed: %lu\n", GetLastError()); return 1; }
    printf("loaded: %p\n", (void*)h);
    void *p = GetProcAddress(h, "fz_new_context_imp");
    printf("fz_new_context_imp: %p\n", p);
    return 0;
}

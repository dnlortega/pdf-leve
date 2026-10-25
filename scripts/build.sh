#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TOOLCHAIN="${PDFLEVE_TOOLCHAIN:-$ROOT/../tools/w64devkit/w64devkit/bin}"
if [ -x "$TOOLCHAIN/gcc.exe" ]; then
  CC="${CC:-$TOOLCHAIN/gcc.exe}"
  CXX="${CXX:-$TOOLCHAIN/g++.exe}"
  WINDRES="${WINDRES:-$TOOLCHAIN/windres.exe}"
else
  CC="${CC:-gcc}"
  CXX="${CXX:-g++}"
  WINDRES="${WINDRES:-windres}"
fi
BUILD="$ROOT/build"
OBJ="$BUILD/obj"
OUT="${PDFLEVE_OUT:-$BUILD/PDFLeve.exe}"

mkdir -p "$OBJ"
cd "$ROOT"

INCLUDES=(
  -Isrc
  -Ithird_party/mupdf/include
  -Ithird_party/cimgui
  -Ithird_party/cimgui/imgui
  -Ithird_party/cimgui/imgui/backends
)
DEFINES=(-D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 -DUNICODE -D_UNICODE)
CFLAGS=(-std=gnu11 -O2 "${DEFINES[@]}" "${INCLUDES[@]}" -Wall -Wextra
  -Wno-unused-parameter -Wno-cast-function-type -Wno-missing-field-initializers
  -finput-charset=UTF-8 -fexec-charset=UTF-8)
CXXFLAGS=(-std=c++17 -O2 "${DEFINES[@]}" "${INCLUDES[@]}" -finput-charset=UTF-8)
STRIP=(-s)
if [ "${DEBUG:-0}" = "1" ]; then
  CFLAGS+=(-g -O0)
  CXXFLAGS+=(-g -O0)
  STRIP=()
fi

build_cpp() {
  local src="$1" obj="$2"
  if [ ! -f "$obj" ] || [ "$src" -nt "$obj" ]; then
    echo "  CXX   $src"
    "$CXX" "${CXXFLAGS[@]}" -c "$src" -o "$obj"
  fi
}

build_c() {
  local src="$1" obj="$2"
  echo "  CC    $src"
  "$CC" "${CFLAGS[@]}" -c "$src" -o "$obj"
}

echo "Compilando PDF Leve..."
echo "  WINDRES resources.rc"
"$WINDRES" -I "$ROOT" "$ROOT/resources.rc" -O coff -o "$OBJ/resources.o"

for name in main app ui_imgui doc state ocr gemini mu; do
  build_c "src/$name.c" "$OBJ/$name.o"
done

build_cpp src/gfx.cpp "$OBJ/gfx.o"
build_cpp third_party/cimgui/cimgui.cpp "$OBJ/cimgui.o"
build_cpp third_party/cimgui/imgui/imgui.cpp "$OBJ/imgui.o"
build_cpp third_party/cimgui/imgui/imgui_draw.cpp "$OBJ/imgui_draw.o"
build_cpp third_party/cimgui/imgui/imgui_tables.cpp "$OBJ/imgui_tables.o"
build_cpp third_party/cimgui/imgui/imgui_widgets.cpp "$OBJ/imgui_widgets.o"
build_cpp third_party/cimgui/imgui/imgui_demo.cpp "$OBJ/imgui_demo.o"
build_cpp third_party/cimgui/imgui/backends/imgui_impl_win32.cpp "$OBJ/imgui_impl_win32.o"
build_cpp third_party/cimgui/imgui/backends/imgui_impl_dx11.cpp "$OBJ/imgui_impl_dx11.o"

echo "  LINK  ${OUT#$ROOT/}"
"$CXX" -municode -mwindows -static-libgcc -static-libstdc++ "${STRIP[@]}" \
  -o "$OUT" \
  "$OBJ/main.o" "$OBJ/app.o" "$OBJ/ui_imgui.o" "$OBJ/doc.o" "$OBJ/state.o" "$OBJ/ocr.o" \
  "$OBJ/gemini.o" "$OBJ/mu.o" "$OBJ/gfx.o" "$OBJ/cimgui.o" "$OBJ/imgui.o" "$OBJ/imgui_draw.o" \
  "$OBJ/imgui_tables.o" "$OBJ/imgui_widgets.o" "$OBJ/imgui_demo.o" "$OBJ/imgui_impl_win32.o" \
  "$OBJ/imgui_impl_dx11.o" "$OBJ/resources.o" \
  -ld3d11 -ldxgi -ld3dcompiler -lwindowscodecs -lgdi32 -luser32 -lshell32 -lole32 -luuid \
  -lcomdlg32 -lshlwapi -ldwmapi -lcomctl32 -lwinhttp -lcrypt32

echo "Pronto: $OUT"

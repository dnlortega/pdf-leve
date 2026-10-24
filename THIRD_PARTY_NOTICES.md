# Avisos de terceiros

O PDF Leve utiliza os seguintes componentes de terceiros.

## MuPDF

- Copyright (c) Artifex Software, Inc.
- Licença: GNU Affero General Public License v3.0 (AGPL-3.0)
- Site: https://mupdf.com/
- Código-fonte: https://github.com/ArtifexSoftware/mupdf
- Versão usada no build: 1.28.2 (Windows x64, `mupdfcpp64.dll`)

O executável publicado inclui o MuPDF. O código-fonte correspondente do PDF Leve
está neste repositório; o código-fonte do MuPDF está disponível no link acima.
O PDF Leve é distribuído sob AGPL-3.0 para atender a esses termos.

## cimgui / Dear ImGui

- cimgui: Copyright (c) 2015 Stephan Dilly — licença MIT (`third_party/cimgui/LICENSE`)
- Dear ImGui: Copyright (c) 2014-2026 Omar Cornut — licença MIT (`third_party/cimgui/imgui/LICENSE.txt`)

## Microsoft Visual C++ Runtime

`msvcp140.dll`, `vcruntime140.dll` e `vcruntime140_1.dll` são redistribuídos
conforme a licença de redistribuição do Microsoft Visual C++ Redistributable.
Esses arquivos não são versionados neste repositório.

## MinGW-w64 / GCC runtime

O executável é compilado com MinGW-w64/GCC e linkado estaticamente com
`libgcc` e `libstdc++`, conforme a GCC Runtime Library Exception.

## Windows OCR

O reconhecimento de texto usa a API `Windows.Media.Ocr` do próprio Windows.
Nenhum dado é enviado para servidores externos.

# MuPDF runtime

Os arquivos DLL do MuPDF não são versionados neste repositório por tamanho e
licença. Para compilar a partir do código-fonte, coloque aqui:

- `mupdfcpp64.dll` (MuPDF 1.28.2, Windows x64)
- `msvcp140.dll`
- `vcruntime140.dll`
- `vcruntime140_1.dll`

O executável publicado na aba **Releases** já é autocontido e extrai essas DLLs
para `%LOCALAPPDATA%\PDF Leve\runtime` no primeiro uso.

MuPDF: https://mupdf.com/ — licença AGPL-3.0.

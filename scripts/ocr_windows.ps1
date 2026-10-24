param(
    [Parameter(Mandatory = $true)][string]$ImagePath,
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [string]$Language = "pt-BR"
)
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Runtime.WindowsRuntime

$asTaskGeneric = ([System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
    $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and
    $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1'
})[0]

function Await($WinRtTask, $ResultType) {
    $asTask = $asTaskGeneric.MakeGenericMethod($ResultType)
    $netTask = $asTask.Invoke($null, @($WinRtTask))
    $netTask.Wait(-1) | Out-Null
    return $netTask.Result
}

[Windows.Storage.StorageFile, Windows.Storage, ContentType = WindowsRuntime] | Out-Null
[Windows.Graphics.Imaging.BitmapDecoder, Windows.Graphics, ContentType = WindowsRuntime] | Out-Null
[Windows.Media.Ocr.OcrEngine, Windows.Media, ContentType = WindowsRuntime] | Out-Null
[Windows.Globalization.Language, Windows.Globalization, ContentType = WindowsRuntime] | Out-Null

$ImagePath = [IO.Path]::GetFullPath($ImagePath)
$OutputPath = [IO.Path]::GetFullPath($OutputPath)

$file = Await ([Windows.Storage.StorageFile]::GetFileFromPathAsync($ImagePath)) ([Windows.Storage.StorageFile])
$stream = Await ($file.OpenAsync([Windows.Storage.FileAccessMode]::Read)) ([Windows.Storage.Streams.IRandomAccessStream])
try {
    $decoder = Await ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])
    $bitmap = Await ($decoder.GetSoftwareBitmapAsync()) ([Windows.Graphics.Imaging.SoftwareBitmap])
    try {
        $engine = $null
        try {
            $languageObject = New-Object Windows.Globalization.Language $Language
            $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage($languageObject)
        } catch {
            $engine = $null
        }
        if ($null -eq $engine) {
            foreach ($available in [Windows.Media.Ocr.OcrEngine]::AvailableRecognizerLanguages) {
                $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage($available)
                if ($null -ne $engine) { break }
            }
        }
        if ($null -eq $engine) {
            throw "Nenhum idioma de OCR instalado no Windows."
        }

        $result = Await ($engine.RecognizeAsync($bitmap)) ([Windows.Media.Ocr.OcrResult])
        $lines = foreach ($line in $result.Lines) { $line.Text }
        [IO.File]::WriteAllText($OutputPath, ($lines -join "`r`n"), (New-Object Text.UTF8Encoding($false)))
    } finally {
        if ($null -ne $bitmap) { $bitmap.Dispose() }
    }
} finally {
    $stream.Dispose()
}

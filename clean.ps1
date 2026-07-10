# Clean script for T# Programming Language - Windows PowerShell
# Dylan Armstrong, 2026

$paths =  ".\build\", ".\generated\"
foreach ($filePath in $paths) {
    if (Test-Path $filePath) {
        Remove-Item $filePath -verbose
    }

    else {
        Write-Host "File path not found: " + $filePath
    }
}
# Build script for T# Programming Language - Windows PowerShell
# Dylan Armstrong, 2026

param(
    [switch]$Clean = $false
)

# Colors
$ErrorColor = "Red"
$SuccessColor = "Green"
$WarningColor = "Yellow"
$InfoColor = "Cyan"

function Write-Status {
    param([string]$Message, [string]$Color = "White")
    Write-Host $Message -ForegroundColor $Color
}

Write-Host ""

# Check for required tools
Write-Status "Checking dependencies..." -Color $InfoColor

$deps = @{
    "cmake" = "CMake (https://cmake.org/download/)";
    "java" = "Java (https://www.java.com/)"
}

$hasMsvc = $null -ne (Get-Command cl.exe -ErrorAction SilentlyContinue)
$hasClang = $null -ne (Get-Command clang.exe -ErrorAction SilentlyContinue)

foreach ($dep in $deps.GetEnumerator()) {
    if ($null -eq (Get-Command $dep.Key -ErrorAction SilentlyContinue)) {
        Write-Status "✗ $($dep.Key) not found" -Color $ErrorColor
        Write-Status "Install $($dep.Value)" -Color $WarningColor
        exit 1
    }
    Write-Status "✓ $($dep.Key)" -Color $SuccessColor
}

# Check for C++ compiler
if (-not ($hasMsvc -or $hasClang)) {
    Write-Status "✗ C++ compiler not found (MSVC or Clang)" -Color $ErrorColor
    Write-Status "Install Visual Studio Build Tools from: https://visualstudio.microsoft.com/" -Color $WarningColor
    exit 1
}
Write-Status "✓ C++ Compiler" -Color $SuccessColor

Write-Host ""
Write-Status "Building T# Compiler..." -Color $InfoColor
Write-Host ""

# Clean if requested
if ($Clean) {
    Write-Status "Cleaning build directory..." -Color $WarningColor
    if (Test-Path "build") {
        Remove-Item "build" -Recurse -Force
    }
}

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Run CMake
Write-Status "Running CMake..." -Color $WarningColor
& cmake . -B build -G "Visual Studio 17 2022" 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Status "✗ CMake configuration failed" -Color $ErrorColor
    exit 1
}

# Build with MSBuild
Write-Status "Running MSBuild..." -Color $WarningColor
& msbuild build\tsharp.sln /p:Configuration=Release /m 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Status "✗ Build failed" -Color $ErrorColor
    exit 1
}

Write-Host ""

# Check if binary was created
$binaryPaths = @(
    ".\Release\tsharp.exe",
    ".\build\Release\tsharp.exe",
    ".\tsharp.exe"
)

$compiler = $null
foreach ($path in $binaryPaths) {
    if (Test-Path $path) {
        $compiler = $path
        break
    }
}

if ($compiler) {
    Write-Status "✓ Build successful!" -Color $SuccessColor
    Write-Host ""
    Write-Status "T# compiler location: $(Resolve-Path $compiler)" -Color $InfoColor
    Write-Host ""
    Write-Status "Ready to use! Try:" -Color $SuccessColor
    Write-Host "  .\test.ps1              # Run test suite"
    Write-Host "  $compiler examples\test.tsharp  # Run example"
    Write-Host ""
    exit 0
} else {
    Write-Status "✗ Build failed - compiler binary not found" -Color $ErrorColor
    exit 1
}

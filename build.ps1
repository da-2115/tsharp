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

# Also check for Visual Studio installations directly
if (-not ($hasMsvc -or $hasClang)) {
    $vsVersions = @("2022", "2019", "2017")
    foreach ($version in $vsVersions) {
        $vsPath = "C:\Program Files\Microsoft Visual Studio\$version\*\VC\Tools\MSVC\*\bin\HostX64\x64\cl.exe"
        if (Test-Path $vsPath) {
            $hasMsvc = $true
            break
        }
    }
}

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

# Bootstrap vcpkg
$vcpkgDir = (Get-Location).Path + "\vcpkg"
if (-not (Test-Path "$vcpkgDir\vcpkg.exe")) {
    Write-Status "Setting up vcpkg..." -Color $InfoColor
    git clone https://github.com/Microsoft/vcpkg.git vcpkg
    & "$vcpkgDir\bootstrap-vcpkg.bat"
    Write-Status "✓ vcpkg ready" -Color $SuccessColor
} else {
    Write-Status "✓ Using cached vcpkg" -Color $SuccessColor
}

# Install dependencies with vcpkg
Write-Status "Installing dependencies..." -Color $InfoColor
& "$vcpkgDir\vcpkg.exe" install --triplet x64-windows
if ($LASTEXITCODE -ne 0) {
    Write-Status "✗ vcpkg install failed" -Color $ErrorColor
    exit 1
}

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
$toolchainPath = "$vcpkgDir\scripts\buildsystems\vcpkg.cmake" -replace '\\', '/'
Write-Status "Using toolchain: $toolchainPath" -Color $InfoColor
& cmake . -B build -G "Visual Studio 17 2022" "-DCMAKE_TOOLCHAIN_FILE=$toolchainPath" 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Status "✗ CMake configuration failed" -Color $ErrorColor
    exit 1
}

# Build with MSBuild
Write-Status "Running MSBuild..." -Color $WarningColor

# Find MSBuild
$msbuild = $null
if (Get-Command msbuild -ErrorAction SilentlyContinue) {
    $msbuild = "msbuild"
} else {
    # Search for MSBuild in Visual Studio installations
    $vsVersions = @("2022", "2019", "2017")
    foreach ($version in $vsVersions) {
        $msPath = "C:\Program Files\Microsoft Visual Studio\$version\*\MSBuild\Current\Bin\MSBuild.exe"
        $found = Get-Item $msPath -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) {
            $msbuild = $found.FullName
            break
        }
    }
}

if (-not $msbuild) {
    Write-Status "✗ MSBuild not found" -Color $ErrorColor
    Write-Status "Please ensure Visual Studio Build Tools are installed" -Color $WarningColor
    exit 1
}

& $msbuild build\tsharp.sln /p:Configuration=Release /m 2>&1

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

# Test script for T# Programming Language - Windows PowerShell
# Dylan Armstrong, 2026

param(
    [switch]$Verbose = $false
)

# Colors
$Script:Colors = @{
    Error = "Red"
    Success = "Green"
    Warning = "Yellow"
    Info = "Cyan"
    Test = "Magenta"
}

# Counters
$Script:TestsRun = 0
$Script:TestsPassed = 0
$Script:TestsFailed = 0
$Script:PassedTests = @()
$Script:FailedTests = @()
$Script:CompilerPath = $null

function Write-Status {
    param([string]$Message, [string]$Color = "White")
    Write-Host $Message -ForegroundColor $Color
}

function Find-Compiler {
    $paths = @(
        ".\Release\tsharp.exe",
        ".\build\Release\tsharp.exe",
        ".\tsharp.exe",
        (Get-Command tsharp.exe -ErrorAction SilentlyContinue).Source
    )
    
    foreach ($path in $paths) {
        if ($path -and (Test-Path $path)) {
            return $path
        }
    }
    return $null
}

function Run-Test {
    param(
        [string]$TestName,
        [string]$Code,
        [string]$Expected
    )
    
    $Script:TestsRun++
    
    # Create temporary file
    $tempFile = [System.IO.Path]::GetTempFileName() -replace '\.tmp$', '.tsharp'
    $code | Set-Content $tempFile -Encoding UTF8
    
    try {
        # Run the T# program
        $output = & $Script:CompilerPath $tempFile 2>&1 | Out-String
        $output = $output -replace "`r`n$", ""  # Remove trailing newline
        
        # Compare output
        if ($output -eq $Expected) {
            Write-Status "✓ PASS: $TestName" -Color $Colors.Success
            $Script:TestsPassed++
            $Script:PassedTests += $TestName
        } else {
            Write-Status "✗ FAIL: $TestName" -Color $Colors.Error
            $Script:FailedTests += $TestName
            if ($Verbose) {
                Write-Status "  Expected: '$Expected'" -Color $Colors.Warning
                Write-Status "  Got:      '$output'" -Color $Colors.Warning
            }
            $Script:TestsFailed++
        }
    }
    finally {
        Remove-Item $tempFile -Force -ErrorAction SilentlyContinue
    }
}

# Main script
Write-Host ""

# Build first
Write-Status "Building T# Compiler..." -Color $Colors.Info
& .\build.ps1
if ($LASTEXITCODE -ne 0) {
    Write-Status "Build failed!" -Color $Colors.Error
    exit 1
}

Write-Host ""

# Find compiler
$Script:CompilerPath = Find-Compiler
if (-not $Script:CompilerPath) {
    Write-Status "Compiler not found!" -Color $Colors.Error
    exit 1
}

Write-Status "Using compiler: $($Script:CompilerPath)" -Color $Colors.Info
Write-Host ""
Write-Status "Running T# Feature Tests" -Color $Colors.Info
Write-Host ""

# Test 1: Hello World
Run-Test "Hello World" @'
void main() {
    println("Hello World!")
}
'@ "Hello World!"

# Test 2: Basic arithmetic
Run-Test "Basic Arithmetic (5+3)" @'
void main() {
    int a = 5
    int b = 3
    println(a + b)
}
'@ "8"

# Test 3: String operations
Run-Test "String Declaration" @'
void main() {
    string name = "T#"
    println(name)
}
'@ "T#"

# Test 4: If/Else
Run-Test "If/Else Statement" @'
void main() {
    int x = 10
    if (x > 5) {
        println("x is greater than 5")
    } else {
        println("x is not greater than 5")
    }
}
'@ "x is greater than 5"

# Test 5: For loop
Run-Test "For Loop (1 to 3)" @'
void main() {
    for (int i = 1; i <= 3; i++) {
        println(i)
    }
}
'@ "1`n2`n3"

# Test 6: While loop
Run-Test "While Loop" @'
void main() {
    int i = 0
    while (i < 3) {
        println(i)
        i++
    }
}
'@ "0`n1`n2"

# Test 7: Function with return
Run-Test "Function with Return Value" @'
int add(int a, int b) {
    return a + b
}

void main() {
    println(add(3, 4))
}
'@ "7"

# Test 8: Factorial
Run-Test "Factorial(5)" @'
void main() {
    println(factorial(5))
}
'@ "120"

# Test 9: Math - abs
Run-Test "Math - Absolute Value" @'
void main() {
    println(abs(-42))
}
'@ "42"

# Test 10: Math - power
Run-Test "Math - Power (2^3)" @'
void main() {
    println(pow(2, 3))
}
'@ "8"

# Test 11: Switch statement
Run-Test "Switch Statement" @'
void main() {
    int day = 2
    switch (day) {
        case 1:
            println("Monday")
            break
        case 2:
            println("Tuesday")
            break
        default:
            println("Other")
    }
}
'@ "Tuesday"

# Test 12: Array initialization
Run-Test "Array Initialization" @'
void main() {
    int arr[3]
    arr[0] = 5
    arr[1] = 10
    arr[2] = 15
    println(arr[0])
    println(arr[1])
    println(arr[2])
}
'@ "5`n10`n15"

# Edge Case: Zero values
Run-Test "Edge Case - Zero Values" @'
void main() {
    int zero = 0
    println(zero)
    println(zero + 5)
    println(zero * 100)
}
'@ "0`n5`n0"

# Edge Case: Negative numbers
Run-Test "Edge Case - Negative Numbers" @'
void main() {
    int neg = -10
    println(neg)
    println(neg + 20)
    println(abs(neg))
}
'@ "-10`n10`n10"

# Edge Case: Factorial edge cases
Run-Test "Edge Case - Factorial(0 and 1)" @'
void main() {
    println(factorial(0))
    println(factorial(1))
    println(factorial(3))
}
'@ "1`n1`n6"

# Summary
Write-Host ""
Write-Status "Test Summary" -Color $Colors.Info
Write-Status "Total Tests:  $($Script:TestsRun)" -Color "White"
Write-Status "Passed:       $($Script:TestsPassed)" -Color $Colors.Success
if ($Script:TestsFailed -gt 0) {
    Write-Status "Failed:       $($Script:TestsFailed)" -Color $Colors.Error
} else {
    Write-Status "Failed:       $($Script:TestsFailed)" -Color $Colors.Success
}

if ($Script:TestsRun -gt 0) {
    $passRate = [math]::Floor(($Script:TestsPassed * 100) / $Script:TestsRun)
    Write-Status "Pass Rate:    $passRate%" -Color $Colors.Warning
}

# Display results
if ($Verbose -or $Script:TestsFailed -gt 0) {
    Write-Host ""
    Write-Status "=== Passed Tests ($($Script:PassedTests.Count)) ===" -Color $Colors.Info
    foreach ($test in $Script:PassedTests) {
        Write-Status "✓ $test" -Color $Colors.Success
    }
    
    if ($Script:TestsFailed -gt 0) {
        Write-Host ""
        Write-Status "=== Failed Tests ($($Script:FailedTests.Count)) ===" -Color $Colors.Info
        foreach ($test in $Script:FailedTests) {
            Write-Status "✗ $test" -Color $Colors.Error
        }
    }
}

# Exit status
Write-Host ""
if ($Script:TestsFailed -gt 0) {
    Write-Status "Some tests failed!" -Color $Colors.Error
    Write-Status "Run with '-Verbose' flag for detailed output" -Color $Colors.Warning
    exit 1
} else {
    Write-Status "All tests passed! ✓" -Color $Colors.Success
    exit 0
}

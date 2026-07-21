param(
    [string]$Cxx = "clang++",
    [switch]$SkipCppfrontBuild
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $PSScriptRoot "build"
New-Item -ItemType Directory -Force -Path $build | Out-Null

if ($SkipCppfrontBuild) {
    if (!(Test-Path (Join-Path $build "cppfront.exe"))) { throw "cppfront.exe is required when -SkipCppfrontBuild is used" }
    Write-Host "Using existing cppfront.exe..."
}
else {
    Write-Host "Building cppfront with $Cxx..."
    & $Cxx -std=c++20 -I (Join-Path $root "include") (Join-Path $root "source/cppfront.cpp") -o (Join-Path $build "cppfront.exe")
    if ($LASTEXITCODE -ne 0) { throw "Failed to build cppfront" }
}

Push-Location $build
try {
    function Invoke-Go2Test([string]$Name, [string[]]$Expected) {
        $source = Join-Path $PSScriptRoot "$Name.go2"
        & .\cppfront.exe $source -output "$Name.cpp" -quiet
        if ($LASTEXITCODE -ne 0) { throw "$Name translation failed" }
        & $Cxx -std=c++20 -I (Join-Path $root "include") "$Name.cpp" -o "$Name.exe"
        if ($LASTEXITCODE -ne 0) { throw "$Name generated C++ did not compile" }
        $actual = & ".\$Name.exe"
        if (@($actual).Count -ne $Expected.Count -or (Compare-Object $actual $Expected)) {
            throw "Unexpected $Name output: $($actual -join ', ')"
        }
        Write-Host "PASS  $Name"
    }

    function Invoke-MixedKeywordTest {
        Copy-Item (Join-Path $PSScriptRoot "mixed_keywords.cpp") mixed_keywords.cpp
        & .\cppfront.exe mixed_keywords.cpp -go2 -quiet
        if ($LASTEXITCODE -ne 0) { throw "Mixed keyword translation failed" }
        if (!(Test-Path mixed_keywords.go2.cpp)) { throw "Default mixed keyword output was not created" }

        $generated = Get-Content -Raw mixed_keywords.go2.cpp
        if ($generated -notmatch '#include "go2util\.h"') { throw "package was not lowered to the Go2 runtime include" }
        if ($generated -notmatch '#include <iostream>') { throw 'import "fmt" was not lowered to the fmt include' }

        & $Cxx -std=c++20 -I (Join-Path $root "include") mixed_keywords.go2.cpp -o mixed_keywords.exe
        if ($LASTEXITCODE -ne 0) { throw "Mixed keyword generated C++ did not compile" }

        $keywords = @(
            "package", "import", "var", "const", "type", "func",
            "if", "else", "for", "range", "switch", "case", "default", "fallthrough",
            "break", "continue", "goto", "return", "defer", "go", "chan", "select",
            "struct", "interface", "map"
        )
        $actual = @(& .\mixed_keywords.exe)
        if ($actual.Count -ne $keywords.Count) {
            throw "Mixed keyword test produced $($actual.Count) results instead of $($keywords.Count)"
        }

        for ($index = 0; $index -lt $keywords.Count; ++$index) {
            if ($actual[$index] -ne "1") {
                throw "$($index + 1). $($keywords[$index]) mixed_keywords failed: $($actual[$index])"
            }
            Write-Host "$($index + 1). $($keywords[$index]) mixed_keywords pass"
        }
    }

    Invoke-Go2Test "hello" @("sum 5", "0", "1", "2")
    Invoke-Go2Test "syntax" @("matched", "count 0", "count 1", "once", "done")
    Invoke-Go2Test "interop" @("42")
    Invoke-Go2Test "complex" @("Ada! 15 8 7 32")
    Invoke-Go2Test "keyword_declarations" @("4")
    Invoke-Go2Test "keyword_switch" @("12 2 99")
    Invoke-Go2Test "keyword_defer" @("body", "deferred")
    Invoke-Go2Test "keyword_channel" @("7")
    Invoke-Go2Test "keyword_select" @("9")

    Copy-Item (Join-Path $PSScriptRoot "mixed.cpp") mixed.cpp
    & .\cppfront.exe mixed.cpp -go2 -quiet
    if ($LASTEXITCODE -ne 0) { throw "Three-way mixed translation failed" }
    if (!(Test-Path mixed.go2.cpp)) { throw "Default mixed output was not created" }
    & $Cxx -std=c++20 -I (Join-Path $root "include") mixed.go2.cpp -o mixed.exe
    if ($LASTEXITCODE -ne 0) { throw "Three-way mixed generated C++ did not compile" }
    $mixedOutput = @(& .\mixed.exe)
    if ($mixedOutput.Count -ne 1 -or $mixedOutput[0] -ne "5 12") {
        throw "Unexpected three-way mixed output: $($mixedOutput -join ', ')"
    }
    Write-Host "PASS  Cpp1 + Cpp2 + Go2 mixed .cpp"

    Copy-Item (Join-Path $PSScriptRoot "mixed_complex.cpp") mixed_complex.cpp
    & .\cppfront.exe mixed_complex.cpp -go2 -quiet
    if ($LASTEXITCODE -ne 0) { throw "Complex three-way mixed translation failed" }
    if (!(Test-Path mixed_complex.go2.cpp)) { throw "Default complex mixed output was not created" }
    & $Cxx -std=c++20 -I (Join-Path $root "include") mixed_complex.go2.cpp -o mixed_complex.exe
    if ($LASTEXITCODE -ne 0) { throw "Complex three-way mixed generated C++ did not compile" }
    $mixedComplexOutput = @(& .\mixed_complex.exe)
    if ($mixedComplexOutput.Count -ne 1 -or $mixedComplexOutput[0] -ne "Ada! 15 32 | Ada! 15 32 | Ada! 15 32") {
        throw "Unexpected complex three-way mixed output: $($mixedComplexOutput -join ', ')"
    }
    Write-Host "PASS  complex Cpp1 + Cpp2 + Go2 mixed .cpp"

    Invoke-MixedKeywordTest

    Copy-Item (Join-Path $PSScriptRoot "hello.go2") hello.go
    & .\cppfront.exe hello.go -go2 -output hello-flag.cpp -quiet
    if ($LASTEXITCODE -ne 0) { throw "Explicit Go2 translation failed" }
    Write-Host "PASS  explicit -go2"

    # PowerShell 7 otherwise promotes this expected nonzero native exit to a terminating error.
    $PSNativeCommandUseErrorActionPreference = $false
    & .\cppfront.exe (Join-Path $PSScriptRoot "unsupported-goroutine.go2") -output unsupported-goroutine.cpp -quiet
    $unsupportedExitCode = $LASTEXITCODE
    if ($unsupportedExitCode -eq 0) { throw "Unsupported anonymous goroutine syntax was accepted" }
    $global:LASTEXITCODE = 0
    Write-Host "PASS  unsupported anonymous goroutine is rejected"
    Write-Host "All Go2 syntax tests passed."
}
finally {
    Pop-Location
}

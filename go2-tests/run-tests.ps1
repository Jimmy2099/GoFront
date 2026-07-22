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

    function Invoke-MixedKeywordComplexTest {
        Copy-Item (Join-Path $PSScriptRoot "mixed_keywords_complex.cpp") mixed_keywords_complex.cpp
        & .\cppfront.exe mixed_keywords_complex.cpp -go2 -quiet
        if ($LASTEXITCODE -ne 0) { throw "Complex keyword three-way mixed translation failed" }
        if (!(Test-Path mixed_keywords_complex.go2.cpp)) { throw "Default complex keyword mixed output was not created" }

        $generated = Get-Content -Raw mixed_keywords_complex.go2.cpp
        if ($generated -notmatch '#include "go2util\.h"') { throw "package was not lowered to the Go2 runtime include" }
        if ($generated -notmatch '#include <iostream>') { throw 'import "fmt" was not lowered to the fmt include' }

        & $Cxx -std=c++20 -I (Join-Path $root "include") mixed_keywords_complex.go2.cpp -o mixed_keywords_complex.exe
        if ($LASTEXITCODE -ne 0) { throw "Complex keyword mixed generated C++ did not compile" }

        $keywords = @(
            "package", "import", "var", "const", "type", "func",
            "if", "else", "for", "range", "switch", "case", "default", "fallthrough",
            "break", "continue", "goto", "return", "defer", "go", "chan", "select",
            "struct", "interface", "map"
        )
        $languages = @("Cpp1", "Cpp2", "Go2")
        $rawOutput = @(& .\mixed_keywords_complex.exe)
        $deferOutput = @($rawOutput | Where-Object { $_ -eq "defer" })
        $actual = @($rawOutput | Where-Object { $_ -ne "defer" })
        if ($deferOutput.Count -ne $languages.Count) {
            throw "Expected $($languages.Count) defer executions, received $($deferOutput.Count)"
        }
        if ($actual.Count -ne ($keywords.Count * $languages.Count)) {
            throw "Mixed keyword complex test produced $($actual.Count) results instead of $($keywords.Count * $languages.Count)"
        }

        $result = 0
        for ($keywordIndex = 0; $keywordIndex -lt $keywords.Count; ++$keywordIndex) {
            for ($languageIndex = 0; $languageIndex -lt $languages.Count; ++$languageIndex) {
                if ($actual[$result] -ne "1") {
                    throw "$($result + 1). $($languages[$languageIndex]) $($keywords[$keywordIndex]) mixed_keywords_complex failed: $($actual[$result])"
                }
                Write-Host "$($result + 1). $($languages[$languageIndex]) $($keywords[$keywordIndex]) mixed_keywords_complex pass"
                ++$result
            }
        }
    }

    function Invoke-MixedTypesComplexTest {
        Copy-Item (Join-Path $PSScriptRoot "mixed_types_complex.cpp") mixed_types_complex.cpp
        & .\cppfront.exe mixed_types_complex.cpp -go2 -quiet
        if ($LASTEXITCODE -ne 0) { throw "Three-way Go data type mixed translation failed" }
        if (!(Test-Path mixed_types_complex.go2.cpp)) { throw "Default Go data type mixed output was not created" }

        $generated = Get-Content -Raw mixed_types_complex.go2.cpp
        $requiredTypes = @(
            "go2::uint",
            "std::uintptr_t",
            "std::complex<float>",
            "std::complex<double>",
            "std::function",
            "std::array",
            "std::vector",
            "std::map",
            "go2::channel"
        )
        foreach ($requiredType in $requiredTypes) {
            if ($generated -notmatch [regex]::Escape($requiredType)) {
                throw "Go data type was not lowered to $requiredType"
            }
        }

        & $Cxx -std=c++20 -I (Join-Path $root "include") mixed_types_complex.go2.cpp -o mixed_types_complex.exe
        if ($LASTEXITCODE -ne 0) { throw "Go data type mixed generated C++ did not compile" }

        $languages = @("Cpp1", "Cpp2", "Go2")
        $actual = @(& .\mixed_types_complex.exe)
        if ($actual.Count -ne $languages.Count) {
            throw "Go data type mixed test produced $($actual.Count) results instead of $($languages.Count)"
        }
        for ($index = 0; $index -lt $languages.Count; ++$index) {
            if ($actual[$index] -ne "1") {
                throw "$($index + 1). $($languages[$index]) mixed_types_complex failed: $($actual[$index])"
            }
            Write-Host "$($index + 1). $($languages[$index]) Go data types mixed_types_complex pass"
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

    Invoke-MixedKeywordComplexTest

    Invoke-MixedTypesComplexTest

    Invoke-MixedKeywordTest

    Copy-Item (Join-Path $PSScriptRoot "hello.go2") hello.go
    & .\cppfront.exe hello.go -go2 -output hello-flag.cpp -quiet
    if ($LASTEXITCODE -ne 0) { throw "Explicit Go2 translation failed" }
    Write-Host "PASS  explicit -go2"

    # This fixture must fail translation. Start-Process keeps its expected nonzero
    # exit from becoming a PowerShell error record while retaining the diagnostic.
    $unsupportedDiagnostic = Join-Path $build "unsupported-goroutine.stderr"
    Remove-Item -LiteralPath $unsupportedDiagnostic -Force -ErrorAction SilentlyContinue
    try {
        $unsupportedProcess = Start-Process `
            -FilePath (Join-Path $build "cppfront.exe") `
            -ArgumentList ('"{0}" -output "unsupported-goroutine.cpp" -quiet' -f (Join-Path $PSScriptRoot "unsupported-goroutine.go2")) `
            -NoNewWindow `
            -PassThru `
            -RedirectStandardError $unsupportedDiagnostic `
            -Wait
        $unsupportedExitCode = $unsupportedProcess.ExitCode
    }
    finally {
        if (Test-Path $unsupportedDiagnostic) {
            Get-Content $unsupportedDiagnostic | ForEach-Object { Write-Host $_ }
            Remove-Item -LiteralPath $unsupportedDiagnostic -Force
        }
    }
    if ($unsupportedExitCode -eq 0) { throw "Unsupported anonymous goroutine syntax was accepted" }
    $global:LASTEXITCODE = 0
    Write-Host "PASS  unsupported anonymous goroutine is rejected"
    Write-Host "All Go2 syntax tests passed."
}
finally {
    Pop-Location
}

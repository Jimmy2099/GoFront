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

    function Invoke-MixedClassesComplexTest {
        Copy-Item (Join-Path $PSScriptRoot "mixed_classes_complex.cpp") mixed_classes_complex.cpp
        & .\cppfront.exe mixed_classes_complex.cpp -go2 -quiet
        if ($LASTEXITCODE -ne 0) { throw "Complex three-way class translation failed" }
        if (!(Test-Path mixed_classes_complex.go2.cpp)) { throw "Default complex class mixed output was not created" }

        $generated = Get-Content -Raw mixed_classes_complex.go2.cpp
        foreach ($requiredClassCode in @(
            "class Go2Ledger {",
            "private: std::vector<int> entries{};",
            "public: std::string Owner{};",
            "private: std::map<std::string, int> totals{};",
            "public: auto Apply(Cpp2Ledger* cpp2, Cpp1Ledger* cpp1, int delta) -> int"
        )) {
            if ($generated -notmatch [regex]::Escape($requiredClassCode)) {
                throw "Go2 class output is missing: $requiredClassCode"
            }
        }

        & $Cxx -std=c++20 -I (Join-Path $root "include") mixed_classes_complex.go2.cpp -o mixed_classes_complex.exe
        if ($LASTEXITCODE -ne 0) { throw "Complex three-way class generated C++ did not compile" }
        $actual = @(& .\mixed_classes_complex.exe)
        if ($actual.Count -ne 1 -or $actual[0] -ne "1 63 63 50 27") {
            throw "Unexpected complex three-way class output: $($actual -join ', ')"
        }
        Write-Host "PASS  complex Cpp1 + Cpp2 + Go2 class mixed .cpp"
    }

    function Invoke-Go2PackageTest {
        $source = Join-Path $PSScriptRoot "packages/app/main.go2"
        & .\cppfront.exe $source -output go2-packages.cpp -quiet
        if ($LASTEXITCODE -ne 0) { throw "Go2 package project translation failed" }
        if (!(Test-Path go2-packages.cpp)) { throw "Go2 package project output was not created" }

        $generated = Get-Content -Raw go2-packages.cpp
        foreach ($requiredNamespace in @(
            "namespace go2pkg_example_2e_com_2f_go2_2d_packages_2f_calc",
            "namespace calc = go2pkg_example_2e_com_2f_go2_2d_packages_2f_calc",
            "namespace score = go2pkg_example_2e_com_2f_go2_2d_packages_2f_labels"
        )) {
            if ($generated -notmatch [regex]::Escape($requiredNamespace)) {
                throw "Go2 package namespace was not emitted: $requiredNamespace"
            }
        }

        & $Cxx -std=c++20 -I (Join-Path $root "include") go2-packages.cpp -o go2-packages.exe
        if ($LASTEXITCODE -ne 0) { throw "Go2 package project generated C++ did not compile" }
        $actual = @(& .\go2-packages.exe)
        if ($actual.Count -ne 1 -or $actual[0] -ne "5 60") {
            throw "Unexpected Go2 package project output: $($actual -join ', ')"
        }
        $lock = Join-Path $PSScriptRoot "packages/go2.lock"
        if (!(Test-Path $lock) -or (Get-Content -Raw $lock) -notmatch [regex]::Escape('path = "example.com/go2-packages"')) {
            throw "Go2 package project did not create its lock file"
        }
        Write-Host "PASS  Go2 module package imports"
    }

    function Invoke-Go2PackageFailureTest {
        $source = Join-Path $PSScriptRoot "packages-invalid/app/main.go2"
        $diagnostic = Join-Path $build "go2-package-import.stderr"
        Remove-Item -LiteralPath $diagnostic -Force -ErrorAction SilentlyContinue
        try {
            $process = Start-Process `
                -FilePath (Join-Path $build "cppfront.exe") `
                -ArgumentList ('"{0}" -output "go2-packages-invalid.cpp" -quiet' -f $source) `
                -NoNewWindow `
                -PassThru `
                -RedirectStandardError $diagnostic `
                -Wait
            if ($process.ExitCode -eq 0) {
                throw "Missing Go2 package import was accepted"
            }
            $message = Get-Content -Raw $diagnostic
            if ($message -notmatch "could not resolve package directory") {
                throw "Unexpected missing Go2 package diagnostic: $message"
            }
        }
        finally {
            Remove-Item -LiteralPath $diagnostic -Force -ErrorAction SilentlyContinue
        }
        Write-Host "PASS  missing Go2 package import is rejected"
    }

    function Invoke-Go2CrossModuleTest {
        $source = Join-Path $PSScriptRoot "modules-cross/app/main.go2"
        & .\cppfront.exe $source -output go2-cross-module.cpp -quiet
        if ($LASTEXITCODE -ne 0) { throw "Cross-module Go2 translation failed" }
        if (!(Test-Path go2-cross-module.cpp)) { throw "Cross-module Go2 output was not created" }

        $generated = Get-Content -Raw go2-cross-module.cpp
        foreach ($requiredNamespace in @(
            "namespace arith = go2pkg_example_2e_com_2f_go2_2d_math_2f_arith",
            "namespace geometry = go2pkg_example_2e_com_2f_go2_2d_shapes_2f_geometry",
            "namespace metrics = go2pkg_example_2e_com_2f_go2_2d_metrics_2f_values",
            "namespace numbers = go2pkg_example_2e_com_2f_go2_2d_numbers_2f_values",
            "cpp1_shift",
            "cpp2_double"
        )) {
            if ($generated -notmatch [regex]::Escape($requiredNamespace)) {
                throw "Cross-module Go2 output is missing: $requiredNamespace"
            }
        }

        & $Cxx -std=c++20 -I (Join-Path $root "include") go2-cross-module.cpp -o go2-cross-module.exe
        if ($LASTEXITCODE -ne 0) { throw "Cross-module Go2 generated C++ did not compile" }
        $actual = @(& .\go2-cross-module.exe)
        if ($actual.Count -ne 1 -or $actual[0] -ne "56 28 3 103") {
            throw "Unexpected cross-module Go2 output: $($actual -join ', ')"
        }

        $lock = Join-Path $PSScriptRoot "modules-cross/app/go2.lock"
        if (!(Test-Path $lock)) { throw "Cross-module Go2 lock file was not created" }
        $lockBefore = Get-Content -Raw $lock
        foreach ($requiredEntry in @(
            'path = "example.com/go2-module-app"',
            'path = "example.com/go2-math"',
            'path = "example.com/go2-metrics"',
            'path = "example.com/go2-numbers"',
            'path = "example.com/go2-shapes"',
            'version = "v1.2.0"',
            'source = "replace:../metrics"',
            'source = "replace:../numbers"'
        )) {
            if ($lockBefore -notmatch [regex]::Escape($requiredEntry)) {
                throw "Cross-module Go2 lock file is missing: $requiredEntry"
            }
        }

        & .\cppfront.exe $source -output go2-cross-module-repeat.cpp -quiet
        if ($LASTEXITCODE -ne 0) { throw "Cross-module Go2 repeat translation failed" }
        $lockAfter = Get-Content -Raw $lock
        if ($lockBefore -cne $lockAfter) { throw "Cross-module Go2 lock file was not deterministic" }
        Write-Host "PASS  Go2 require + replace + transitive version resolution"
    }

    function Invoke-Go2VendorModuleTest {
        $source = Join-Path $PSScriptRoot "modules-vendor/app/main.go2"
        & .\cppfront.exe $source -output go2-vendor.cpp -quiet
        if ($LASTEXITCODE -ne 0) { throw "Vendored Go2 translation failed" }
        & $Cxx -std=c++20 -I (Join-Path $root "include") go2-vendor.cpp -o go2-vendor.exe
        if ($LASTEXITCODE -ne 0) { throw "Vendored Go2 generated C++ did not compile" }
        $actual = @(& .\go2-vendor.exe)
        if ($actual.Count -ne 1 -or $actual[0] -ne "15") {
            throw "Unexpected vendored Go2 output: $($actual -join ', ')"
        }
        $lock = Get-Content -Raw (Join-Path $PSScriptRoot "modules-vendor/app/go2.lock")
        if ($lock -notmatch [regex]::Escape('source = "vendor"')) {
            throw "Vendored Go2 lock file did not record the vendor source"
        }
        Write-Host "PASS  Go2 vendored module imports"
    }

    function Invoke-Go2ManifestDirectiveTest {
        $source = Join-Path $PSScriptRoot "modules-manifest/app/main.go2"
        & .\cppfront.exe $source -output go2-manifest.cpp -quiet
        if ($LASTEXITCODE -ne 0) { throw "Go2 manifest directive translation failed" }
        & $Cxx -std=c++20 -I (Join-Path $root "include") go2-manifest.cpp -o go2-manifest.exe
        if ($LASTEXITCODE -ne 0) { throw "Go2 manifest directive generated C++ did not compile" }
        $actual = @(& .\go2-manifest.exe)
        if ($actual.Count -ne 1 -or $actual[0] -ne "21") {
            throw "Unexpected Go2 manifest directive output: $($actual -join ', ')"
        }
        $lock = Get-Content -Raw (Join-Path $PSScriptRoot "modules-manifest/app/go2.lock")
        if ($lock -notmatch [regex]::Escape('path = "example.com/go2-manifest-lib"')) {
            throw "Go2 manifest directive lock file was not created"
        }
        Write-Host "PASS  Go2 grouped manifest directives"
    }

    function Invoke-Go2ExcludedModuleFailureTest {
        $source = Join-Path $PSScriptRoot "modules-exclude/app/main.go2"
        $diagnostic = Join-Path $build "go2-excluded-module.stderr"
        Remove-Item -LiteralPath $diagnostic -Force -ErrorAction SilentlyContinue
        try {
            $process = Start-Process `
                -FilePath (Join-Path $build "cppfront.exe") `
                -ArgumentList ('"{0}" -output "go2-excluded-module.cpp" -quiet' -f $source) `
                -NoNewWindow `
                -PassThru `
                -RedirectStandardError $diagnostic `
                -Wait
            if ($process.ExitCode -eq 0) {
                throw "Excluded Go2 module version was accepted"
            }
            $message = Get-Content -Raw $diagnostic
            if ($message -notmatch "is excluded by") {
                throw "Unexpected excluded Go2 module diagnostic: $message"
            }
        }
        finally {
            Remove-Item -LiteralPath $diagnostic -Force -ErrorAction SilentlyContinue
        }
        Write-Host "PASS  Go2 excluded module version is rejected"
    }

    function Invoke-Go2ModuleMixedSourceTest {
        $source = Join-Path $PSScriptRoot "modules-cross/app/mixed/main.cpp"
        $lockPath = Join-Path $PSScriptRoot "modules-cross/app/go2.lock"
        $lockBefore = Get-Content -Raw $lockPath
        & .\cppfront.exe $source -go2 -output go2-module-mixed.cpp -quiet
        if ($LASTEXITCODE -ne 0) { throw "Mixed Go2 module translation failed" }
        $generated = Get-Content -Raw go2-module-mixed.cpp
        foreach ($requiredSymbol in @(
            "namespace arith = go2pkg_example_2e_com_2f_go2_2d_math_2f_arith",
            "cpp1_seed",
            "cpp2_factor",
            "go2_apply"
        )) {
            if ($generated -notmatch [regex]::Escape($requiredSymbol)) {
                throw "Mixed Go2 module output is missing: $requiredSymbol"
            }
        }
        & $Cxx -std=c++20 -I (Join-Path $root "include") go2-module-mixed.cpp -o go2-module-mixed.exe
        if ($LASTEXITCODE -ne 0) { throw "Mixed Go2 module generated C++ did not compile" }
        $actual = @(& .\go2-module-mixed.exe)
        if ($actual.Count -ne 1 -or $actual[0] -ne "56") {
            throw "Unexpected mixed Go2 module output: $($actual -join ', ')"
        }
        if ($lockBefore -cne (Get-Content -Raw $lockPath)) {
            throw "Mixed Go2 module entry changed the module lock file"
        }

        $plainSource = Join-Path $PSScriptRoot "modules-cross/app/mixed/plain.cpp"
        & .\cppfront.exe $plainSource -go2 -output go2-module-plain.cpp -quiet
        if ($LASTEXITCODE -ne 0) { throw "Plain C++ module translation regressed" }
        & $Cxx -std=c++20 -I (Join-Path $root "include") go2-module-plain.cpp -o go2-module-plain.exe
        if ($LASTEXITCODE -ne 0) { throw "Plain C++ module generated C++ did not compile" }
        $plainActual = @(& .\go2-module-plain.exe)
        if ($plainActual.Count -ne 1 -or $plainActual[0] -ne "8") {
            throw "Unexpected plain C++ module output: $($plainActual -join ', ')"
        }
        Write-Host "PASS  Cpp1 + Cpp2 + Go2 module entry"
    }

    function Invoke-Go2RemoteModuleFailureTest {
        $source = Join-Path $PSScriptRoot "modules-invalid/app/main.go2"
        $diagnostic = Join-Path $build "go2-remote-module.stderr"
        Remove-Item -LiteralPath $diagnostic -Force -ErrorAction SilentlyContinue
        try {
            $process = Start-Process `
                -FilePath (Join-Path $build "cppfront.exe") `
                -ArgumentList ('"{0}" -output "go2-remote-module.cpp" -quiet' -f $source) `
                -NoNewWindow `
                -PassThru `
                -RedirectStandardError $diagnostic `
                -Wait
            if ($process.ExitCode -eq 0) {
                throw "Remote Go2 module import was accepted"
            }
            $message = Get-Content -Raw $diagnostic
            if ($message -notmatch "remote downloads are disabled") {
                throw "Unexpected remote Go2 module diagnostic: $message"
            }
        }
        finally {
            Remove-Item -LiteralPath $diagnostic -Force -ErrorAction SilentlyContinue
        }
        Write-Host "PASS  Go2 remote module import is rejected"
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

    Invoke-Go2PackageTest
    Invoke-Go2PackageFailureTest
    Invoke-Go2CrossModuleTest
    Invoke-Go2VendorModuleTest
    Invoke-Go2ManifestDirectiveTest
    Invoke-Go2ModuleMixedSourceTest
    Invoke-Go2ExcludedModuleFailureTest
    Invoke-Go2RemoteModuleFailureTest

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

    Invoke-MixedClassesComplexTest

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

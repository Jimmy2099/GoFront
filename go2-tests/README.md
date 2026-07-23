# Go2 tests

This project verifies the experimental Go-inspired C++ input path end to end:

```powershell
.\run-tests.ps1
```

After a successful build, `.\run-tests.ps1 -SkipCppfrontBuild` reuses `build\cppfront.exe` for quicker repeated test runs.

It builds cppfront, translates each supported-syntax fixture to C++, compiles the result, and checks its output. It also verifies that unsupported anonymous goroutine syntax is rejected. The default compiler is `clang++`; pass `-Cxx <compiler>` to select another C++20 compiler.

Go2 package projects use a Go-style `go2.mod` manifest, with `go.mod` accepted for compatibility. It supports `module`, `go`, `toolchain`, single-line and grouped `require`, local `replace`, `exclude`, and compatibility parsing for `retract`. The resolver walks the active requirement graph before source assembly, selects the highest declared version for each local module, and uses only the root module's `replace` entries, matching Go's main-module behavior. A dependency must be supplied by a local `replace` or under `vendor/<module-path>`; remote downloads and third-party registries are intentionally not part of the compiler.

Every successful module translation writes a deterministic `go2.lock`. It records the workspace and resolved modules, their selected versions, local source (`replace` or `vendor`), manifest paths, and normalized-source fingerprints. `go2.lock` is deliberately not ignored, so applications can commit it like `Cargo.lock`.

`interop.go2` verifies that Go2 files still use cppfront's existing mixed-source support: native Cpp2 declarations can appear directly, and a `//go2:cpp1-begin` / `//go2:cpp1-end` region is passed through as Cpp1.

`mixed.cpp` verifies a traditional C++ source file that contains Cpp1, Cpp2, and Go2 together. Run it with `cppfront mixed.cpp -go2`; the generated output is `mixed.go2.cpp` by default.

`complex.go2` covers Go2 structures passed and modified by value and pointer, string fields, slices and fixed arrays with indexed reads/writes, and an empty `map[string]int` populated through subscript assignment.

`mixed_complex.cpp` implements the same complex structure, pointer, container, string, and dictionary operation separately in Cpp1, Cpp2, and Go2, then compares all three results from a Cpp1 `main`.

`mixed_classes_complex.cpp` exercises Go2's `type Name class` syntax. The Go2 class has private string, slice, fixed-array, map, and revision state plus exported Go-style `func` member methods; lowercase identifiers are private and uppercase identifiers are public. Cpp1 starts the call, the Go2 class calls Cpp2, and the Cpp2 class calls back into Cpp1. The fixture verifies both native C++ class lowering and Cpp1/Cpp2/Go2 pointer and member-call interoperability.

`mixed_keywords_complex.cpp` runs a 75-case (`Cpp1`, `Cpp2`, and `Go2` entry points for each of Go's 25 reserved keywords) shared-global interoperability matrix. The workspace contains nested structures, a structure array, an integer array, and a dictionary. Each case returns through Go2 -> Cpp2 -> Cpp1, with every stage reading and updating the same global data. The test runner reports `1` through `75` with language, keyword, and `pass`.

`mixed_types_complex.cpp` defines a Go2 structure containing booleans; fixed-width signed and unsigned integers; `byte`, `rune`, `uint`, `uintptr`; floating-point and complex numbers; strings; arrays; slices; maps; and a function value. It also exercises pointer parameters, interfaces, and channels. Cpp1, Cpp2, and Go2 each enter the same Go2 -> Cpp2 -> Cpp1 shared-data call chain, and the runner reports one result for each entry language.

`packages/` is a multi-package module test. Its `app` package imports `calc` by the default package name and imports `labels` as `score`; `labels` itself imports `calc` and accepts a `calc.Pair` across the package boundary. The imported `calc` package also contains a Cpp1 passthrough function called by Go2 code.

`packages-invalid/` verifies that a module-relative import with no package directory is rejected during source loading.

`modules-cross/` verifies a full multi-module graph: grouped directives, `go` and `toolchain` metadata, version-specific local replacements, transitive requirements, highest-version selection, lock-file stability, collision-safe package namespaces, and Cpp1/Cpp2/Go2 code in the imported call chain. Its `mixed/main.cpp` additionally verifies a package-declaring traditional C++ entry file that calls Go2, Cpp2, and imported module code without losing the existing mixed-source behavior; `mixed/plain.cpp` verifies the no-package fallback to cppfront's existing mixed-source path.

`modules-vendor/` verifies the local vendor fallback and its lock-file source record. `modules-invalid/` verifies that an unresolved remote requirement is rejected instead of being downloaded implicitly.

`modules-manifest/` verifies grouped `require`, `replace`, and `retract` directives together with `go` and `toolchain` metadata. `modules-exclude/` verifies that a root-module `exclude` directive rejects the selected local version before source loading.

`mixed_keywords.cpp` is a traditional `.cpp` file containing Cpp1, Cpp2, and Go2. It executes each of Go's 25 reserved keywords through real syntax, including a native-label fallback for Go2 `goto`, and exercises the Cpp1 -> Go2 -> Cpp2 -> Cpp1 call chain. The test runner reports each keyword as `N. keyword mixed_keywords pass`.

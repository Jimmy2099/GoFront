# Go2 tests

This project verifies the experimental Go-inspired C++ input path end to end:

```powershell
.\run-tests.ps1
```

After a successful build, `.\run-tests.ps1 -SkipCppfrontBuild` reuses `build\cppfront.exe` for quicker repeated test runs.

It builds cppfront, translates each supported-syntax fixture to C++, compiles the result, and checks its output. It also verifies that unsupported anonymous goroutine syntax is rejected. The default compiler is `clang++`; pass `-Cxx <compiler>` to select another C++20 compiler.

`interop.go2` verifies that Go2 files still use cppfront's existing mixed-source support: native Cpp2 declarations can appear directly, and a `//go2:cpp1-begin` / `//go2:cpp1-end` region is passed through as Cpp1.

`mixed.cpp` verifies a traditional C++ source file that contains Cpp1, Cpp2, and Go2 together. Run it with `cppfront mixed.cpp -go2`; the generated output is `mixed.go2.cpp` by default.

`complex.go2` covers Go2 structures passed and modified by value and pointer, string fields, slices and fixed arrays with indexed reads/writes, and an empty `map[string]int` populated through subscript assignment.

`mixed_complex.cpp` implements the same complex structure, pointer, container, string, and dictionary operation separately in Cpp1, Cpp2, and Go2, then compares all three results from a Cpp1 `main`.

`mixed_keywords_complex.cpp` runs a 75-case (`Cpp1`, `Cpp2`, and `Go2` entry points for each of Go's 25 reserved keywords) shared-global interoperability matrix. The workspace contains nested structures, a structure array, an integer array, and a dictionary. Each case returns through Go2 -> Cpp2 -> Cpp1, with every stage reading and updating the same global data. The test runner reports `1` through `75` with language, keyword, and `pass`.

`mixed_types_complex.cpp` defines a Go2 structure containing booleans; fixed-width signed and unsigned integers; `byte`, `rune`, `uint`, `uintptr`; floating-point and complex numbers; strings; arrays; slices; maps; and a function value. It also exercises pointer parameters, interfaces, and channels. Cpp1, Cpp2, and Go2 each enter the same Go2 -> Cpp2 -> Cpp1 shared-data call chain, and the runner reports one result for each entry language.

`mixed_keywords.cpp` is a traditional `.cpp` file containing Cpp1, Cpp2, and Go2. It executes each of Go's 25 reserved keywords through real syntax, including a native-label fallback for Go2 `goto`, and exercises the Cpp1 -> Go2 -> Cpp2 -> Cpp1 call chain. The test runner reports each keyword as `N. keyword mixed_keywords pass`.

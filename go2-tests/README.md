# Go2 tests

This project verifies the experimental Go-inspired C++ input path end to end:

```powershell
.\run-tests.ps1
```

It builds cppfront, translates each supported-syntax fixture to C++, compiles the result, and checks its output. It also verifies that unsupported goroutine syntax is rejected. The default compiler is `clang++`; pass `-Cxx <compiler>` to select another C++20 compiler.

`interop.go2` verifies that Go2 files still use cppfront's existing mixed-source support: native Cpp2 declarations can appear directly, and a `//go2:cpp1-begin` / `//go2:cpp1-end` region is passed through as Cpp1.

`mixed.cpp` verifies a traditional C++ source file that contains Cpp1, Cpp2, and Go2 together. Run it with `cppfront mixed.cpp -go2`; the generated output is `mixed.go2.cpp` by default.

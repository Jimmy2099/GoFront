# Go2 modules and packages

Go2 source can be organized as Go-style packages when an input file is below a `go2.mod` manifest. A standard `go.mod` is also accepted, which makes it practical to migrate an existing local layout while keeping Go2-specific build output separate.

A traditional `.cpp` entry supplied with `-go2` remains supported inside a module when it contains a Go2 `package` declaration. It can combine Cpp1, Cpp2, and Go2 while importing resolved Go2 packages; ordinary `.cpp` files without a package declaration continue through cppfront's existing mixed-source path.

```text
module example.com/app

go 1.23.0

require (
    example.com/math v1.2.0
)

replace example.com/math v1.2.0 => ../math
```

An import below the current module path resolves to a package directory in that module. Other imports resolve through the active requirement graph. Go2 supports `module`, `go`, `toolchain`, `require`, `replace`, `exclude`, and `retract` directives, including grouped `require`, `replace`, `exclude`, and `retract` forms. `retract` is accepted as compatibility metadata because Go2 never queries a remote version index.

`replace` is local-only. It may target a specific version or every version of a module. As in Go module mode, only the root module's replacements affect the build. In the absence of a replacement, Go2 searches:

```text
vendor/<module-import-path>/go2.mod
```

Remote registries and automatic network downloads are intentionally unsupported. This keeps a cppfront translation self-contained and makes build inputs explicit.

Before translating packages, Go2 resolves the complete local requirement graph and deterministically picks the highest declared version for each module. Imported packages are emitted in generated C++ namespaces derived from their full import paths, so two modules may safely contain packages with the same package name. Go selectors are lowered to those namespaces while package aliases retain their Go spelling in source.

Each successful module build writes `go2.lock` beside the root manifest. The lock file records the selected modules, versions, local origins, manifest paths, and normalized-source fingerprints. Commit it for executable projects and CI builds, in the same spirit as `Cargo.lock`.

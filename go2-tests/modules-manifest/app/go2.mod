module example.com/go2-manifest-app

go 1.23.0
toolchain go1.23.0

require (
    example.com/go2-manifest-lib v0.1.0
)

replace (
    example.com/go2-manifest-lib v0.1.0 => ../lib
)

retract (
    v0.0.1
    [v0.0.2, v0.0.3]
)

module example.com/go2-module-app

go 1.23.0
toolchain go1.23.0

require (
    example.com/go2-math v1.1.0
    example.com/go2-metrics v0.5.0
    example.com/go2-numbers v1.0.0
    example.com/go2-shapes v1.0.0
)

replace (
    example.com/go2-math v1.1.0 => ../math
    example.com/go2-metrics => ../metrics
    example.com/go2-numbers v1.0.0 => ../numbers
    example.com/go2-numbers v1.2.0 => ../numbers
    example.com/go2-shapes => ../shapes
)

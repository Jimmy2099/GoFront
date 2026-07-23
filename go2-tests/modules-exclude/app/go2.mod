module example.com/go2-exclude-app

require (
    example.com/go2-exclude-lib v0.1.0
)

exclude (
    example.com/go2-exclude-lib v0.1.0
)

replace example.com/go2-exclude-lib => ../lib

# Code style
cpp should be limited to test/ (including __cplusplus guards). don't pollute the C code with CPPisms

use `const char *` to represent human readable strings like paths. use `uint8_t*` for binary data like hashes

this project is Windows-only, compiled using MSVC via CMake, targeting modern x86-64-v2. Use standard Win32 / Windows APIs and MSVC runtime functions where appropriate. Do not add POSIX-only dependencies or cosmocc compatibility code.
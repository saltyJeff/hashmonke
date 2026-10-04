# Code style
cpp should be limited to test/ (including __cplusplus guards). don't pollute the C code with CPPisms

use `const char *` to represent human readable strings like paths. use `uint8_t*` for binary data like hashes

do not do windows dependent (e.g. #ifdef _WIN32) code (exception: tui.h/tui.c may minimally use ifdef guards for conio vs termios terminal mode handling). this is compiled under cosmocc/mingw, so windows APIs won't work elsewhere.
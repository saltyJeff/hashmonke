.PHONY: all test clean

ifeq ($(origin CC),default)
CC := clang
endif
ifeq ($(origin CXX),default)
CXX := clang++
endif
CFLAGS ?= -std=c17 -march=x86-64-v3 -Wall -Wextra -O3 -pthread -I.
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O3 -pthread -I.
LDFLAGS ?= -pthread
STRIP ?= strip

USE_ASM ?= 1
TARGET_TRIPLE ?= $(shell $(CC) -dumpmachine)
X86_64_TARGET := $(filter x86_64%,$(TARGET_TRIPLE))
ifeq ($(USE_ASM),0)
  ALGO_OBJS = algo/md_internal.o algo/md5_c.o algo/sha1_c.o algo/crc32_c.o
else
ifeq ($(X86_64_TARGET),)
  ALGO_OBJS = algo/md_internal.o algo/md5_c.o algo/sha1_c.o algo/crc32_c.o
else
  ALGO_OBJS = algo/md_internal.o algo/md5_asm.o algo/md5_x86_64.o algo/sha1_asm.o algo/sha1_x86_64.o
  ifeq ($(OS),Windows_NT)
    ALGO_OBJS += algo/crc32_asm.o algo/crc32_x86_64.o
  else
    ALGO_OBJS += algo/crc32_c.o
  endif
endif
endif

ALL_ALGO_OBJS = algo/md_internal.o algo/md5_c.o algo/sha1_c.o algo/crc32_c.o \
	algo/md5_asm.o algo/md5_x86_64.o algo/sha1_asm.o algo/sha1_x86_64.o \
	algo/crc32_asm.o algo/crc32_x86_64.o

CORE_OBJS = file.o file_sfv.o file_gnu.o file_bsd.o hasher.o runner.o cli.o
TEST_OBJS = test/main.o test/test_md5.o test/test_sha1.o test/test_crc32.o test/test_file.o test/test_hasher.o test/test_runner.o test/test_cli.o test/test_e2e.o
DEPFILES = $(CORE_OBJS:.o=.d) $(TEST_OBJS:.o=.d) $(ALGO_OBJS:.o=.d) main.d
TEST_BIN = test/test_hashmonke
BIN = hashmonke
ALL_DEPFILES = $(CORE_OBJS:.o=.d) $(TEST_OBJS:.o=.d) $(ALL_ALGO_OBJS:.o=.d) main.d

all: $(BIN)

$(BIN): main.o $(CORE_OBJS) $(ALGO_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	$(STRIP) --strip-debug $@$(if $(filter Windows_NT,$(OS)),.exe,)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_OBJS) $(ALGO_OBJS) $(CORE_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

algo/%_c.o: algo/%.c
	$(CC) $(CFLAGS) -DMD5_NO_ASM -DSHA1_NO_ASM -DCRC32_NO_ASM -MMD -MP -MF $(@:.o=.d) -c $< -o $@

algo/%_asm.o: algo/%.c
	$(CC) $(CFLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

clean:
ifeq ($(OS),Windows_NT)
	-del /f /q $(subst /,\,$(ALL_ALGO_OBJS) $(CORE_OBJS) $(TEST_OBJS) $(ALL_DEPFILES) main.o $(BIN) $(BIN).exe $(TEST_BIN) $(TEST_BIN).exe) 2>nul
else
	-rm -f $(ALL_ALGO_OBJS) $(CORE_OBJS) $(TEST_OBJS) $(ALL_DEPFILES) main.o $(BIN) $(BIN).exe $(TEST_BIN) $(TEST_BIN).exe
endif

-include $(DEPFILES)


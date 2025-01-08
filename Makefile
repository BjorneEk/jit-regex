UNAME:=$(shell uname -s)

BIN:=bin/$(UNAME)
TEST_BIN:=$(BIN)/test

TARGET:=$(BIN)/target

BUILD:=build/$(UNAME)
TEST_BUILD:=$(BIN)/test/

LIB:=lib/$(UNAME)
INCLUDE:=include

SRC:=src
TEST:=test
TEST_COMMON:=$(TEST)/common

MAIN:=main.c

SOURCE_C:=$(wildcard $(SRC)/*.c $(SRC)/*/*.c)
SOURCE_H:=$(wildcard $(SRC)/*.h $(SRC)/*/*.h)

TEST_C:=$(wildcard $(TEST_COMMON)/*.c $(TEST_COMMON)/*/*.c)
TEST_H:=$(wildcard $(TEST_COMMON)/*.h $(TEST_COMMON)/*/*.h)
TEST_TESTS:=$(wildcard $(TEST)/*.c)


SOURCE_OBJ:=$(patsubst $(SRC)/%.c, $(BUILD)/%.o, $(SOURCE_C))
TEST_OBJ:=$(patsubst $(TEST_COMMON)/%.c, $(TEST_BUILD)/%.o, $(TEST_C))

TESTS:=$(patsubst $(TEST)/%.c, $(TEST_BIN)/%, $(TEST_TESTS))

SOURCE_OBJ_MSG_PRINTED:=1
TEST_OBJ_MSG_PRINTED:=1
TEST_MSG_PRINTED:=1
MAIN_MSG_PRINTED:=1

LIBS:=$(patsubst $(LIB)/%, -L%, $(wildcard $(LIB)/*))

CFLAGS:=-std=c2x -pedantic -Wall -Werror -Wno-newline-eof -Wno-gnu-binary-literal -Wno-unused-function $(LIBS) -g -I$(INCLUDE) -I$(SRC)
SRC_CFLAGS:=-flto -O3
TEST_CFLAGS:=-flto -O3 -I$(TEST_COMMON)

ifeq ($(UNAME), Linux)
	CC:=gcc
	DEBUG:=gdb -tui
	CHECK:=valgrind --leak-check=full --show-reachable=yes -s
else
	CC:=clang
	DEBUG:=lldb
endif

all: $(TARGET) Makefile

.PHONY: clean

$(BUILD)/%.o: $(SRC)/%.c $(SOURCE_H)
	@mkdir -p $(dir $@)

	$(if $(filter 0,$(MAKELEVEL)), $(if $(filter 0,$(SOURCE_OBJ_MSG_PRINTED)),, \
	$(eval SOURCE_OBJ_MSG_PRINTED:=0) \
	@echo "\nCompiling object files"))

	@$(CC) $(CFLAGS) $(SRC_CFLAGS) -c $< -o $@
	@printf " - %-50s <- %s\n" "$@" " $<"

$(TARGET): $(MAIN) $(SOURCE_OBJ)
	@mkdir -p $(dir $@)

	$(if $(filter 0,$(MAKELEVEL)), $(if $(filter 0,$(MAIN_MSG_PRINTED)),, \
	$(eval MAIN_MSG_PRINTED:=0) \
	@echo "\nCompiling target"))

	@$(CC) $(CFLAGS) $(SRC_CFLAGS) -o $@ $^
	@printf " - %-50s <- %s\n" "$@" " $^"

$(TEST_BUILD)/%.o: $(TEST_COMMON)/%.c $(SOURCE_H) $(TEST_H)
	@mkdir -p $(dir $@)

	$(if $(filter 0,$(MAKELEVEL)), $(if $(filter 0,$(TEST_OBJ_MSG_PRINTED)),, \
	$(eval TEST_OBJ_MSG_PRINTED:=0) \
	@echo "\nCompiling test common files"))

	@$(CC) $(CFLAGS) $(TEST_CFLAGS) -c $< -o $@
	@printf " - %-50s <- %s\n" "$@" " $<"

$(TEST_BIN)/%: $(TEST)/%.c $(SOURCE_OBJ) $(TEST_OBJ)
	@mkdir -p $(dir $@)

	$(if $(filter 0,$(MAKELEVEL)), $(if $(filter 0,$(TEST_MSG_PRINTED)),, \
	$(eval TEST_MSG_PRINTED:=0) \
	@echo "\nCompiling tests"))

	@$(CC) $(CFLAGS) $(TEST_CFLAGS) -o $@ $^
	@printf " - %-50s <- %s\n" "$@" " $^"

run-tests: $(TESTS)
	@mkdir -p files
	@echo "\nRunning tests\n"
	@pass=0; fail=0; \
	green='\033[32m'; red='\033[31m'; reset='\033[0m'; \
	for test in $(TESTS); do \
		if $$test; then \
			printf "%-50s %bPASSED%b\n" "Test $$(basename $$test):" "$${green}" "$${reset}"; \
			pass=$$((pass+1)); \
		else \
			printf "%-50s %bFAILED%b\n" "Test $$(basename $$test):" "$${red}" "$${reset}"; \
			fail=$$((fail+1)); \
		fi; \
	done; \
	echo "\nSummary: $${green}$$pass passed$${reset}, $${red}$$fail failed$${reset}";

run: $(TARGET)
	./$<

print_vars:
	@printf "%-50s %s\n" "UNAME"		"$(UNAME)"
	@printf "%-50s %s\n" "BIN"		"$(BIN)"
	@printf "%-50s %s\n" "TEST_BIN"		"$(TEST_BIN)"
	@printf "%-50s %s\n" "TARGET"		"$(TARGET)"
	@printf "%-50s %s\n" "BUILD"		"$(BUILD)"
	@printf "%-50s %s\n" "TEST_BUILD"	"$(TEST_BUILD)"
	@printf "%-50s %s\n" "LIB"		"$(LIB)"
	@printf "%-50s %s\n" "INCLUDE"		"$(INCLUDE)"
	@printf "%-50s %s\n" "SRC"		"$(SRC)"
	@printf "%-50s %s\n" "TEST"		"$(TEST)"
	@printf "%-50s %s\n" "TEST_COMMON"	"$(TEST_COMMON)"
	@printf "%-50s %s\n" "MAIN"		"$(MAIN)"
	@printf "%-50s %s\n" "SOURCE_C"		"$(SOURCE_C)"
	@printf "%-50s %s\n" "SOURCE_H"		"$(SOURCE_H)"
	@printf "%-50s %s\n" "TEST_C"		"$(TEST_C)"
	@printf "%-50s %s\n" "TEST_H"		"$(TEST_H)"
	@printf "%-50s %s\n" "TEST_TESTS"	"$(TEST_TESTS)"
	@printf "%-50s %s\n" "SOURCE_OBJ"	"$(SOURCE_OBJ)"
	@printf "%-50s %s\n" "TEST_OBJ"		"$(TEST_OBJ)"
	@printf "%-50s %s\n" "TESTS"		"$(TESTS)"
	@printf "%-50s %s\n" "CFLAGS"		"$(CFLAGS)"
	@printf "%-50s %s\n" "SRC_CFLAGS"	"$(SRC_CFLAGS)"
	@printf "%-50s %s\n" "TEST_CFLAGS"	"$(TEST_CFLAGS)"
	@printf "%-50s %s\n" "CC"		"$(CC)"
	@printf "%-50s %s\n" "DEBUG"		"$(DEBUG)"
	@printf "%-50s %s\n" "CHECK"		"$(CHECK)"

clean:
	rm -rf $(BIN)
	rm -rf $(BUILD)

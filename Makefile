SRC=src
BUILD=build
TEST=test

COMPILER=gcc
OPTIONS=-Wall -Wextra -Wshadow -Wconversion -Wunreachable-code -g
# -pedantic -Werror
TEST_OPTIONS=-l cmocka -L /usr/lib/
# TEST_OPTIONS=-l cmocka -L /usr/local/Cellar/cmocka
COMPILE=$(COMPILER) $(OPTIONS)

COMMON_HEADERS = $(SRC)/cpu.h $(SRC)/io.h $(SRC)/memory.h
ASSEM_HEADERS = $(SRC)/assem.h $(SRC)/table.h

default: build bbb

$(BUILD)/machine.o: $(SRC)/cpu.c $(COMMON_HEADERS)
	$(COMPILE) -c $< -o $@

$(BUILD)/memory.o: $(SRC)/memory.c $(COMMON_HEADERS)
	$(COMPILE) -c $< -o $@

$(BUILD)/table.o: $(SRC)/table.c $(ASSEM_HEADERS)
	$(COMPILE) -c $< -o $@

$(BUILD)/assem.o: $(SRC)/assem.c $(ASSEM_HEADERS)
	$(COMPILE) -c $< -o $@

$(BUILD)/io.o: $(SRC)/io.c $(COMMON_HEADERS)
	$(COMPILE) -c $< -o $@

test: $(SRC)/cpu.c $(SRC)/memory.c $(SRC)/io.c $(SRC)/assem.c $(SRC)/table.c
	$(COMPILE) $(TEST)/main.c $(TEST_OPTIONS) -g -o $@.out

bbb: $(BUILD)/machine.o $(BUILD)/memory.o $(BUILD)/io.o $(BUILD)/table.o $(BUILD)/assem.o $(SRC)/main.c
	$(COMPILE) $^ -o $@

build:
	mkdir -p $(BUILD)

clean:
	rm -r $(BUILD)/*

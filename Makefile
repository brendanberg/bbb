SRC=src
BUILD=build

COMPILER=gcc
OPTIONS=-Wall -g
# OPTIONS=-pedantic -Wall -Wextra -Werror -Wshadow -Wconversion -Wunreachable-code -g
COMPILE=$(COMPILER) $(OPTIONS)

COMMON_HEADERS = $(SRC)/machine/cpu.h $(SRC)/machine/io.h $(SRC)/machine/memory.h $(SRC)/machine/sim.h
ASSEM_HEADERS = $(SRC)/assem/assem.h $(SRC)/assem/table.h

default: build bbb

$(BUILD)/machine.o: $(SRC)/machine/cpu.c $(COMMON_HEADERS)
	$(COMPILE) -c $< -o $@

$(BUILD)/memory.o: $(SRC)/machine/memory.c $(COMMON_HEADERS)
	$(COMPILE) -c $< -o $@

$(BUILD)/table.o: $(SRC)/assem/table.c $(ASSEM_HEADERS)
	$(COMPILE) -c $< -o $@

$(BUILD)/assem.o: $(SRC)/assem/assem.c $(ASSEM_HEADERS)
	$(COMPILE) -c $< -o $@

$(BUILD)/io.o: $(SRC)/machine/io.c $(COMMON_HEADERS)
	$(COMPILE) -c $< -o $@

$(BUILD)/sim.o: $(SRC)/machine/sim.c $(COMMON_HEADERS)
	$(COMPILE) -c $< -o $@

bbb: $(BUILD)/machine.o $(BUILD)/memory.o $(BUILD)/io.o $(BUILD)/sim.o $(BUILD)/table.o $(BUILD)/assem.o $(SRC)/main.c
	$(COMPILE) $^ -o $@

build:
	mkdir -p $(BUILD)

test: $(BUILD)/munit.o $(BUILD)/machine.o $(BUILD)/memory.o $(BUILD)/io.o $(BUILD)/table.o $(BUILD)/assem.o $(SRC)/test/*.c $(SRC)/test.c
	$(COMPILE) $^ -o $@

$(BUILD)/munit.o: $(SRC)/munit/munit.c $(SRC)/munit/munit.h
	$(COMPILE) -c $< -o $@

clean:
	rm -r $(BUILD)/*

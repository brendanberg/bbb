SRC=src
BUILD=build

COMPILER=gcc
OPTIONS=-Wall
# -pedantic -Wall -Wextra -Werror -Wshadow -Wconversion -Wunreachable-code
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

bbb: $(BUILD)/machine.o $(BUILD)/memory.o $(BUILD)/io.o $(BUILD)/table.o $(BUILD)/assem.o $(SRC)/main.c
	$(COMPILE) $^ -o $@

build:
	mkdir -p $(BUILD)

clean:
	rm -r $(BUILD)/*

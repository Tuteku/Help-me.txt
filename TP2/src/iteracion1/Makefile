# Makefile — TP2: Cálculos en ensamblador con API REST
# Toolchain: gcc + GNU as (x86-64 Linux)

CC     = gcc
AS     = as
CFLAGS = -g -O0 -Wall
ASFLAGS= --64 -g

TARGET  = programa
SRCS_C  = main.c
SRCS_S  = asm_functions.s
OBJS    = main.o asm_functions.o

.PHONY: all run clean

all: $(TARGET)

asm_functions.o: asm_functions.s
	$(AS) $(ASFLAGS) -o $@ $<

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

# Ejecutar el pipeline completo: Python -> C -> stdout
run: $(TARGET)
	python3 api_client.py | ./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

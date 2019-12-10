CROSSCOMPILE ?= riscv64-unknown-elf-
CC = ${CROSSCOMPILE}gcc
PYTHON=python

CFLAGS = -Os -ggdb -march=rv64imac -mabi=lp64 -Wall -mcmodel=medany -mexplicit-relocs
CCASFLAGS = -mcmodel=medany -mexplicit-relocs
LDFLAGS = -nostdlib -nodefaultlibs -nostartfiles

ifneq ("$(wildcard $(BOARD))", "")
	PLATFORM:=$(BOARD)
else
	PLATFORM:=default
endif

XILFLASH_DIR=xilflash_src
XILFLASH_INCLUDES = -I$(XILFLASH_DIR) -I$(XILFLASH_DIR)/include
XILFLASH_A = $(XILFLASH_DIR)/libxilflash.a

INCLUDES = -I./ -I./src -I$(PLATFORM) $(XILFLASH_INCLUDES)

SRCS_C = src/main.c src/uart.c src/spi.c src/sd.c src/gpt.c src/cdecode.c
SRCS_ASM = startup.S
OBJS_C = $(SRCS_C:.c=.o)
OBJS_S = $(SRCS_ASM:.S=.o)

MAIN = bootrom.elf
MAIN_BIN = $(MAIN:.elf=.bin)
MAIN_IMG = $(MAIN:.elf=.img)
MAIN_SV = $(MAIN:.elf=.sv)

#.PHONY: clean

$(MAIN): ariane.dtb $(OBJS_C) $(OBJS_S) linker.lds $(XILFLASH_A)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -Tlinker.lds $(OBJS_S) $(OBJS_C) $(XILFLASH_A) -o $(MAIN)
	@echo "LD    >= $(MAIN)"

$(XILFLASH_A):
	cd $(XILFLASH_DIR) && make

%.img: %.bin
	dd if=$< of=$@ bs=128

%.bin: %.elf
	$(CROSSCOMPILE)objcopy -O binary $< $@

%.o: %.c
	@$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@
	@echo "CC    <= $<"

%.o: %.S
	@$(CC) $(CFLAGS) $(CCASFLAGS) $(INCLUDES) -c $<  -o $@
	@echo "CC    <= $<"

%.dtb: $(PLATFORM)/%.dts
	dtc -I dts $< -O dtb -o $@

%.sv: %.img
	$(PYTHON) ./gen_rom.py $<
	@echo "PYTHON >= $(MAIN_SV)"

clean:
	$(RM) $(OBJS_C) $(OBJS_S) $(MAIN) $(MAIN_BIN) $(MAIN_IMG) *.dtb
	cd $(XILFLASH_DIR) && make clean

all: $(MAIN) $(MAIN_BIN) $(MAIN_IMG) $(MAIN_SV)
	@echo "zero stage bootloader has been compiled!"

# DO NOT DELETE THIS LINE -- make depend needs it

#
# CC1110 SDCC & Linux Makefile
#
# Requires: sdcc, packihx, cc-tool + dependancies
#
# Discovered at: http://paulswasteland.blogspot.com/2015/01/building-your-own-firmware-for-ciseco.html
#
# Note: Makefiles are super sensitive to the tab character
#
SOURCE = sensor-main

# Tools / Executables 
COMPILER = sdcc
HEXMAKER = packihx
CCUPLOADER = cc-tool

COMPILE_FLAGS = --model-small --opt-code-speed

#Super important that the addresses are appropriately offset.
LDFLAGS_FLASH = \
	--out-fmt-ihx \
	--code-loc 0x000 --code-size 0x8000 \
	--xram-loc 0xf000 --xram-size 0x300 \
	--iram-size 0x100
ifdef DEBUG
COMPILE_FLAGS += --debug
endif
SRC = $(SOURCE).c
ASM=$(SRC:.c=.asm)
IHX=$(SRC:.c=.ihx)
LK=$(SRC:.c=.lk)
LST=$(SRC:.c=.lst)
PMAP=$(SRC:.c=.map)
PMEM=$(SRC:.c=.mem)
REL=$(SRC:.c=.rel)
RST=$(SRC:.c=.rst)
SYM=$(SRC:.c=.sym)
HEX=$(SRC:.c=.hex)
#%.rel : %.c
#	$(CC) -c $(COMPILE_FLAGS) -o$*.rel $<

# Compile using SDCC
all: $(PROGS)
	#$(TARGET).hex: $(REL) Makefile
	$(COMPILER) $(LDFLAGS_FLASH) $(COMPILE_FLAGS) $(SRC)
	$(HEXMAKER) $(IHX) > $(HEX)

# Upload to cc1110 using cc-tool
upload:
	sudo cc-tool -e -w $(HEX)
	
# Clean up
clean:
	rm -f $(ASM) $(IHX) $(LK) $(LST) $(PMAP) $(PMEM) $(REL) $(RST) $(SYM)

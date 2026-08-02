EE_BIN = amongus.elf

EE_OBJS = \
	src/main.o \
	src/game.o

EE_INCS = \
	-I./tyra/inc

EE_LIBS = \
	-ltyra \
	-lpatches \
	-lgskit \
	-ldma \
	-lgraph \
	-ldraw \
	-lpacket \
	-lpng \
	-lz

all: $(EE_BIN)

clean:
	rm -f $(EE_OBJS)
	rm -f $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
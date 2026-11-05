EE_BIN = amongus.elf

EE_OBJS = \
	src/main.o \
	src/game.o

# Headers do PS2SDK, incluindo as bibliotecas de ports (libpng).
EE_INCS = \
    -I./tyra/inc \
    -I$(PS2SDK)/ee/include \
    -I$(PS2SDK)/common/include \
    -I$(PS2SDK)/ports/include

# As bibliotecas de ports ficam em PS2SDK/ports/lib.
EE_LIBS = \
    -L$(PS2SDK)/ports/lib \
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

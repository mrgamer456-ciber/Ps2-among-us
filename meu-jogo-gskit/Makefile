# Projeto de teste PS2SDK + gsKit
EE_BIN = primeiro_teste.elf
EE_OBJS = src/main.o

# O gsKit instalado pelo PS2DEV fica nos diretórios ports.
EE_INCS += -I$(PS2SDK)/ports/include
EE_LDFLAGS += -L$(PS2SDK)/ports/lib
EE_LIBS += -lgskit -ldmakit

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

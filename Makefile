TARGET := amongus.elf
ENGINEDIR := tyra

SRCDIR := src
INCDIR := src
BUILDDIR := obj
TARGETDIR := bin
RESDIR := assets
SRCEXT := cpp
VSMEXT := vsm
VCLEXT := vcl
VCLPPEXT := vclpp
DEPEXT := d
OBJEXT := o

CFLAGS :=
LIB := -ltyra
LIBDIRS := -L$(ENGINEDIR)/bin
INC := -I$(INCDIR) -I$(ENGINEDIR)/inc
INCDEP := -I$(INCDIR) -I$(ENGINEDIR)/inc

all: build-engine

build-engine:
	$(MAKE) clean all -C $(ENGINEDIR)

include Makefile.base

.PHONY: build-engine

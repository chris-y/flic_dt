MAKE := make
CC   := gcc
CP  := copy
RM   := rm -f

CFLAGS := -O3 -funroll-loops -ffast-math -fomit-frame-pointer -mcrt=clib2
# -gstabs
LINK := -nostartfiles

# Additional linker libraries
LIBS :=

# Name of the "thing" to build
TARGET  := flic.datatype
VERSION := 1

OBJS := init.o class.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LINK) -o $@ $^ $(LIBS)

.PHONY: install clean revision

install: $(TARGET)
	$(CP) $(TARGET) SYS:Classes/DataTypes/

clean:
	$(RM) $(TARGET) $(OBJS)

revision:
	bumprev $(VERSION) $(TARGET)


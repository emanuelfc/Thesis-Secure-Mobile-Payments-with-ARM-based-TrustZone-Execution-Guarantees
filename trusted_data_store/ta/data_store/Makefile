CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
AR      = $(CROSS_COMPILE)ar
RANLIB  = $(CROSS_COMPILE)ranlib
NM      = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
READELF = $(CROSS_COMPILE)readelf

OBJS = data_store.o data.o ../hashtable/entry.o ../hashtable/prime.o ../hashtable/hashtable.o ../hashtable/hashtable_iterator.o

LIBNAME = libdata_store

CFLAGS += -I./include

# Hashtable includes
CFLAGS += -I../hashtable/include

#Add/link other required libraries here
LDFLAGS += -L../hashtable
LDLIBS += -lhashtable

LIBFILE = $(LIBNAME).a

.PHONY: all
all: $(LIBFILE)

$(LIBFILE): $(OBJS)
	$(AR) crs $@ $(OBJS)
	$(RANLIB) $@

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(LIBFILE)

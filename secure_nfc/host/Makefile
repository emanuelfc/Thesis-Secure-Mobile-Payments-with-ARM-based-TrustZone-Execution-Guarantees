CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
AR      = $(CROSS_COMPILE)ar
RANLIB  = $(CROSS_COMPILE)ranlib
NM      = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
READELF = $(CROSS_COMPILE)readelf

OBJS = snfc.o snfc_ta_entry.o

LIBNAME = libsnfc

CFLAGS += -Wall -I$(TEEC_EXPORT)/include -I./include -I../ta/include -I/home/thesis/Desktop/optee_rpi3bplus/out-br/build/libnfc-1.7.1/include

#Add/link other required libraries here
LDLIBS += -L$(TEEC_EXPORT)/lib
LDLIBS += -lteec

LDLIBS += -L/home/thesis/Desktop/optee_rpi3bplus/out-br/build/libnfc-1.7.1/libnfc
LDLIBS += -lnfc

LIBFILE = $(LIBNAME).a

.PHONY: all
all: $(LIBFILE)

$(LIBFILE): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(LIBFILE)

CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
AR      = $(CROSS_COMPILE)ar
NM      = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
READELF = $(CROSS_COMPILE)readelf

OBJS = hce_server.o ../../common/hce.o

CFLAGS += -DTRUSTED_HCE_SERVER

CFLAGS += -Wall -I$(TEEC_EXPORT)/include
CFLAGS += -I../../common/include 
CFLAGS += -I/home/thesis/Desktop/optee_rpi3bplus/out-br/build/libnfc-1.7.1/include
CFLAGS += -I/home/thesis/Desktop/secure_nfc/host/include

#Add/link other required libraries here
LDADD += -L$(TEEC_EXPORT)/lib -lteec

# nfc
LDADD += -L/home/thesis/Desktop/optee_rpi3bplus/out-br/build/libusb-1.0.22/libusb/.libs -lusb
LDADD += -L/home/thesis/Desktop/optee_rpi3bplus/out-br/build/libusb-compat-0.1.5/libusb/.libs -lusb-0.1
LDADD += -L/home/thesis/Desktop/optee_rpi3bplus/out-br/build/libnfc-1.7.1/libnfc/.libs -lnfc
LDADD += -L/home/thesis/Desktop/optee_rpi3bplus/out-br/build/pcsc-lite-1.8.23/src/.libs -lpcsclite

LDADD += -L/home/thesis/Desktop/secure_nfc/host -lsnfc

BINARY = trusted_hce_server

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) -o $@ ../../common/hce.o $< $(LDADD)

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINARY)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


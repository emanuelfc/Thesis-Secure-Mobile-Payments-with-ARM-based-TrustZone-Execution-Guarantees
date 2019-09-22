CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
AR      = $(CROSS_COMPILE)ar
NM      = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
READELF = $(CROSS_COMPILE)readelf

OBJS = evaluate_thce_client.o evaluate_uhce_client.o trusted_hce_ta_entry.o ../../common/hce.o /home/thesis/Desktop/trusted_data_store/ta/data_store/data_store_storage/data_store_storage.o

CFLAGS += -Wall -I$(TEEC_EXPORT)/include
CFLAGS += -I../../common/include
CFLAGS += -I./include
CFLAGS += -I../../ta/include
CFLAGS += -I/home/thesis/Desktop/optee_rpi3bplus/out-br/build/libnfc-1.7.1/include
CFLAGS += -I/home/thesis/Desktop/secure_nfc/host/include
CFLAGS += -I/home/thesis/Desktop/trusted_data_store/ta/hashtable/include
CFLAGS += -I/home/thesis/Desktop/trusted_data_store/ta/data_store/include
CFLAGS += -I/home/thesis/Desktop/trusted_data_store/ta/data_store/data_store_storage/include

#Add/link other required libraries here
LDADD += -L$(TEEC_EXPORT)/lib -lteec
LDADD += -L/home/thesis/Desktop/optee_rpi3bplus/out-br/build/libusb-1.0.22/libusb/.libs -lusb
LDADD += -L/home/thesis/Desktop/optee_rpi3bplus/out-br/build/libusb-compat-0.1.5/libusb/.libs -lusb-0.1
LDADD += -L/home/thesis/Desktop/optee_rpi3bplus/out-br/build/libnfc-1.7.1/libnfc/.libs -lnfc
LDADD += -L/home/thesis/Desktop/optee_rpi3bplus/out-br/build/pcsc-lite-1.8.23/src/.libs -lpcsclite
LDADD += -L/home/thesis/Desktop/secure_nfc/host -lsnfc

LDADD += -L/home/thesis/Desktop/trusted_data_store/ta/data_store -ldata_store
LDADD += -L/home/thesis/Desktop/trusted_data_store/ta/hashtable -lhashtable

BINARY = evaluate_thce_client evaluate_uhce_client

.PHONY: all
all: $(BINARY)

evaluate_thce_client: evaluate_thce_client.o trusted_hce_ta_entry.o ../../common/hce.o
	$(CC) -o evaluate_thce_client evaluate_thce_client.o trusted_hce_ta_entry.o ../../common/hce.o $(LDADD)

evaluate_uhce_client: evaluate_uhce_client.o ../../common/hce.o /home/thesis/Desktop/trusted_data_store/ta/data_store/data_store_storage/data_store_storage.o
	$(CC) -o evaluate_uhce_client evaluate_uhce_client.o ../../common/hce.o /home/thesis/Desktop/trusted_data_store/ta/data_store/data_store_storage/data_store_storage.o $(LDADD)

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINARY)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


CC      ?= gcc
LD      ?= ld
AR      ?= ar
NM      ?= nm
OBJCOPY ?= objcopy
OBJDUMP ?= objdump
READELF ?= readelf

OBJS = test_data_store.o ../data_store_storage/data_store_storage.o

CFLAGS += -I../include
CFLAGS += -I../data_store_storage/include
CFLAGS += -I../../hashtable/include

#Add/link other required libraries here
LDFLAGS += -L/home/thesis/Desktop/trusted_data_store/ta/data_store
LDLIBS += -ldata_store
LDFLAGS += -L/home/thesis/Desktop/trusted_data_store/ta/hashtable
LDLIBS += -lhashtable

BINARY = test_data_store

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) $(LDLIBS)

.PHONY: clean
clean:	
	rm -f $(OBJS) $(BINARY)

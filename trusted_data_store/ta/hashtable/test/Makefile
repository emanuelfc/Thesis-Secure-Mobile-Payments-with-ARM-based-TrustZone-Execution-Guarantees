CC      ?= gcc
LD      ?= ld
AR      ?= ar
NM      ?= nm
OBJCOPY ?= objcopy
OBJDUMP ?= objdump
READELF ?= readelf

#src = $(wildcard *.c)
#obj = $(src:.c=.o)

OBJS = test_hashtable.o testing_utils.o

# Hashtable Includes
CFLAGS += -I../include

# Utils Includes
CFLAGS += -I./

#Add/link other required libraries here
LDFLAGS += -L../
LDLIBS += -lhashtable

BINARY = test_hashtable

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) $(LDLIBS)

.PHONY: clean
clean:	
	rm -f $(OBJS) $(BINARY)

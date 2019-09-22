global-incdirs-y += include
global-incdirs-y += ./hashtable/include
global-incdirs-y += ./data_store/include

srcs-y += trusted_data_store.c
srcs-y += tds_storage.c

# Hashtable
libnames += hashtable
libdirs += ./hashtable/

# Data Store
libnames += data_store
libdirs += ./data_store/

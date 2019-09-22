global-incdirs-y += include
global-incdirs-y += ../common/include
global-incdirs-y += ../../../../../../home/thesis/Desktop/secure_nfc/host/include
global-incdirs-y += ../../../../../../home/thesis/Desktop/trusted_data_store/host/include
#global-incdirs-y += ../../../../../../home/thesis/Desktop/optee_rpi3bplus/optee_client/out/export/usr/include

global-incdirs-y += ../../../../../../home/thesis/Desktop/trusted_data_store/ta/include
srcs-y += ../../../../../../home/thesis/Desktop/trusted_data_store/ta/tds_ta_to_ta_entry.c

global-incdirs-y += ../../../../../../home/thesis/Desktop/secure_nfc/ta/include
srcs-y += ../../../../../../home/thesis/Desktop/secure_nfc/ta/snfc_ta_to_ta_entry.c

srcs-y += ../common/hce.c
srcs-y += trusted_hce.c
srcs-y += trusted_hce_ta.c

# To remove a certain compiler flag, add a line like this
#cflags-template_ta.c-y += -Wno-strict-prototypes

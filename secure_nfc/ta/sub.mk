global-incdirs-y += include
global-incdirs-y += ../host/include

srcs-y += snfc_ta_utils.c
srcs-y += snfc_ta_authentication.c
srcs-y += snfc_ta_handshake.c
srcs-y += snfc_ta_communication.c
srcs-y += snfc_ta.c

# To remove a certain compiler flag, add a line like this
#cflags-template_ta.c-y += -Wno-strict-prototypes

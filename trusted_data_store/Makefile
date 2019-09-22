export V?=0

CFG_ARM64_core=y
CFG_TEE_BENCHMARK=y
CFG_TEE_CORE_LOG_LEVEL=4
CROSS_COMPILE=/home/thesis/Desktop/optee_rpi3bplus/toolchains/aarch64/bin/aarch64-linux-gnu-
CROSS_COMPILE_core=/home/thesis/Desktop/optee_rpi3bplus/toolchains/aarch64/bin/aarch64-linux-gnu-
CROSS_COMPILE_ta_arm32=/home/thesis/Desktop/optee_rpi3bplus/toolchains/aarch32/bin/arm-linux-gnueabihf-
CROSS_COMPILE_ta_arm64=/home/thesis/Desktop/optee_rpi3bplus/toolchains/aarch64/bin/aarch64-linux-gnu-
DEBUG=1
O=out/arm
PLATFORM=rpi3
#PLATFORM=vexpress-qemu_virt
TEEC_EXPORT=/home/thesis/Desktop/optee_rpi3bplus/optee_client/out/export/usr
TA_DEV_KIT_DIR=/home/thesis/Desktop/optee_rpi3bplus/optee_os/out/arm/export-ta_arm64
#TEEC_EXPORT=/home/thesis/Desktop/optee_qemu/optee_client/out/export/usr
#TA_DEV_KIT_DIR=/home/thesis/Desktop/optee_qemu/optee_os/out/arm/export-ta_arm64

# If _HOST or _TA specific compilers are not specified, then use CROSS_COMPILE
HOST_CROSS_COMPILE ?= $(CROSS_COMPILE)
TA_CROSS_COMPILE ?= $(CROSS_COMPILE)

.PHONY: all
all:
	#$(MAKE) -C host CROSS_COMPILE="$(HOST_CROSS_COMPILE)" TEEC_EXPORT=$(TEEC_EXPORT) --no-builtin-variables
	$(MAKE) -C ta CROSS_COMPILE="$(TA_CROSS_COMPILE)" LDFLAGS="" TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR)

.PHONY: clean
clean:
	$(MAKE) -C host clean
	$(MAKE) -C ta clean TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR)


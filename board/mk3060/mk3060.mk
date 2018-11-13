ifeq ($(AOS_2BOOT_SUPPORT), yes)
NAME := board_mk3060_2boot
$(NAME)_MBINS_TYPE := kernel
$(NAME)_VERSION := 0.0.1
$(NAME)_SUMMARY :=
else
NAME := board_mk3060
endif


JTAG := jlink


MODULE               := EMW3060
HOST_ARCH            := ARM968E-S
HOST_MCU_FAMILY      := moc108
SUPPORT_BINS         := no

ifeq ($(AOS_2BOOT_SUPPORT), yes)
$(NAME)_SOURCES := flash_partitions.c
else
$(NAME)_SOURCES := board.c flash_partitions.c
endif

GLOBAL_INCLUDES += .
GLOBAL_DEFINES += STDIO_UART=0

CONFIG_SYSINFO_PRODUCT_MODEL := ALI_AOS_MK3060
CONFIG_SYSINFO_DEVICE_NAME := MK3060

# OTA Board config
# 0:OTA_RECOVERY_TYPE_DIRECT 1:OTA_RECOVERY_TYPE_ABBACK 2:OTA_RECOVERY_TYPE_ABBOOT
AOS_SDK_2BOOT_SUPPORT := yes
ifeq ($(AOS_2BOOT_SUPPORT), yes)
GLOBAL_CFLAGS += -DAOS_OTA_RECOVERY_TYPE=1 
GLOBAL_CFLAGS += -DAOS_OTA_2BOOT_CLI
endif
GLOBAL_CFLAGS += -DAOS_OTA_BANK_SINGLE

GLOBAL_CFLAGS += -DSYSINFO_PRODUCT_MODEL=\"$(CONFIG_SYSINFO_PRODUCT_MODEL)\"
GLOBAL_CFLAGS += -DSYSINFO_DEVICE_NAME=\"$(CONFIG_SYSINFO_DEVICE_NAME)\"
#GLOBAL_CFLAGS += -DSYSINFO_APP_VERSION=\"$(CONFIG_SYSINFO_APP_VERSION)\"

ifeq ($(BINS),)
GLOBAL_LDS_INCLUDES += $(SOURCE_ROOT)/board/mk3060/memory.ld.S
else ifeq ($(BINS),app)
GLOBAL_LDS_INCLUDES += $(SOURCE_ROOT)/board/mk3060/memory_app.ld.S
else ifeq ($(BINS),framework)
GLOBAL_LDS_INCLUDES += $(SOURCE_ROOT)/board/mk3060/memory_framework.ld.S
else ifeq ($(BINS),kernel)
GLOBAL_LDS_INCLUDES += $(SOURCE_ROOT)/board/mk3060/memory_kernel.ld.S
endif

GLOBAL_2BOOT_LDS_INCLUDES = $(SOURCE_ROOT)/board/mk3060/memory_2boot.ld.S

# Extra build target in mico_standard_targets.mk, include bootloader, and copy output file to eclipse debug file (copy_output_for_eclipse)
EXTRA_TARGET_MAKEFILES +=  $(MAKEFILES_PATH)/aos_standard_targets.mk
EXTRA_TARGET_MAKEFILES +=  $(SOURCE_ROOT)/platform/mcu/$(HOST_MCU_FAMILY)/gen_crc_bin.mk

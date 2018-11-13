
NAME := board_frdmkl81z
$(NAME)_MBINS_TYPE := kernel
$(NAME)_VERSION := 0.0.1
$(NAME)_SUMMARY :=

MODULE              := 81
HOST_ARCH           := Cortex-M0
HOST_MCU_FAMILY     := mkl81z7

CONFIG_SYSINFO_PRODUCT_MODEL := ALI_AOS_MKL81Z7
CONFIG_SYSINFO_DEVICE_NAME := MKL81Z7
GLOBAL_CFLAGS += -DSYSINFO_PRODUCT_MODEL=\"$(CONFIG_SYSINFO_PRODUCT_MODEL)\"
GLOBAL_CFLAGS += -DSYSINFO_DEVICE_NAME=\"$(CONFIG_SYSINFO_DEVICE_NAME)\"
GLOBAL_CFLAGS += -DSYSINFO_ARCH=\"$(HOST_ARCH)\"
GLOBAL_CFLAGS += -DSYSINFO_MCU=\"$(HOST_MCU_FAMILY)\"
GLOBAL_CFLAGS += -DCONFIG_NO_TCPIP
GLOBAL_CFLAGS += -DFRDM_KL81Z -DFREEDOM
GLOBAL_LDFLAGS += 

GLOBAL_INCLUDES += .
GLOBAL_INCLUDES += ../../platform/mcu/mkl81z7/
GLOBAL_INCLUDES += ../../platform/mcu/mkl81z7/CMSIS/Include
GLOBAL_INCLUDES += ../../platform/mcu/mkl81z7/drivers


$(NAME)_SOURCES     :=
$(NAME)_SOURCES     += ./board.c
$(NAME)_SOURCES     += ./clock_config.c
$(NAME)_SOURCES     += ./pin_mux.c

TEST_COMPONENTS += certificate
GLOBAL_CFLAGS += -DTEST_CONFIG_KV_ENABLED=0
GLOBAL_CFLAGS += -DTEST_CONFIG_YLOOP_ENABLED=1

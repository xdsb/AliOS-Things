
NAME := board_frdmkl27z
$(NAME)_MBINS_TYPE := kernel
$(NAME)_VERSION := 0.0.1
$(NAME)_SUMMARY :=

MODULE              := 27
HOST_ARCH           := Cortex-M0
HOST_MCU_FAMILY     := mkl27z644

CONFIG_SYSINFO_PRODUCT_MODEL := ALI_AOS_MKL27Z644
CONFIG_SYSINFO_DEVICE_NAME := MKL27Z644
GLOBAL_CFLAGS += -DSYSINFO_PRODUCT_MODEL=\"$(CONFIG_SYSINFO_PRODUCT_MODEL)\"
GLOBAL_CFLAGS += -DSYSINFO_DEVICE_NAME=\"$(CONFIG_SYSINFO_DEVICE_NAME)\"
GLOBAL_CFLAGS += -DSYSINFO_ARCH=\"$(HOST_ARCH)\"
GLOBAL_CFLAGS += -DSYSINFO_MCU=\"$(HOST_MCU_FAMILY)\"
GLOBAL_CFLAGS += -DCONFIG_NO_TCPIP
GLOBAL_CFLAGS += -DFRDM_KL27Z -DFREEDOM
GLOBAL_LDFLAGS += 

GLOBAL_INCLUDES += .
GLOBAL_INCLUDES += ../../platform/mcu/mkl27z644/
GLOBAL_INCLUDES += ../../platform/mcu/mkl27z644/CMSIS/Include
GLOBAL_INCLUDES += ../../platform/mcu/mkl27z644/drivers


$(NAME)_SOURCES     :=
$(NAME)_SOURCES     += ./board.c
$(NAME)_SOURCES     += ./clock_config.c
$(NAME)_SOURCES     += ./pin_mux.c

TEST_COMPONENTS += certificate
GLOBAL_CFLAGS += -DTEST_CONFIG_KV_ENABLED=0
GLOBAL_CFLAGS += -DTEST_CONFIG_YLOOP_ENABLED=1
